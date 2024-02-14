// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci.hpp>
#include <drivers/proc.hpp>
#include <drivers/smp.hpp>

#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <module.hpp>

#include "nvme.hpp"

namespace nvme
{
    std::vector<std::unique_ptr<Controller>> ctrls;

    bool Registers::toggle(bool enable)
    {
        uint32_t value = enable;

        this->ctrlconf.en = value;

        size_t start = time::time_ms();
        size_t end = this->caps.to * 500;

        while (start < end)
        {
            if (this->ctrlstat.rdy == value)
                break;

            arch::pause();
            start++;
        }

        return this->ctrlstat.cfs == 0 && this->ctrlstat.rdy == value;
    }

    template<typename Der, typename Type>
    Queue<Der, Type>::Queue(Registers *regs, uint16_t id, size_t size) :
        queue_size(size), phys_allocator(), index(0)
    {
        auto strides = regs->caps.dstrd;
        auto doorbell_offset = getdboffset(id, strides);

        auto base = reinterpret_cast<uintptr_t>(regs);
        this->doorbell = reinterpret_cast<uint32_t*>(base + doorbell_offset);

        auto pptr = this->phys_allocator.allocate(size);
        this->queue = new(tohh(pptr)) Type[size]();
        this->phys_queue = reinterpret_cast<uintptr_t>(pptr);
    }

    template<typename Der, typename Type>
    Queue<Der, Type>::~Queue()
    {
        this->phys_allocator.deallocate(reinterpret_cast<Type*>(this->phys_queue), this->queue_size);
    }

    void SubmissionQueue::submit_command(spec::Command cmd)
    {
        const_cast<spec::Command &>(this->queue[this->index]) = cmd;
        this->index = (this->index + 1) % this->queue_size;
        *this->doorbell = this->index;
    }

    std::expected<spec::CompletionEntry, uint16_t> CompletionQueue::next_cmd_result()
    {
        auto &cmd = this->queue[this->index];

        while ((cmd.status & 0x01) != uint16_t(this->phase))
            arch::pause();
        // cmd.status &= ~0x01; // use phases instead

        auto status = cmd.status >> 1;
        if (status != 0)
            return std::unexpected(status);

        this->index = (this->index + 1) % this->queue_size;
        if (this->index == 0)
            this->phase = !this->phase;

        *this->doorbell = this->index;
        return const_cast<spec::CompletionEntry &>(cmd);
    }

    bool QueuePair::submit_command_nolock(spec::Command cmd)
    {
        // size_t next_cid = 0;
        // for (size_t i = 0; i < this->cid_bitmap.length(); i++)
        // {
        //     if (this->cid_bitmap[i] == bitmap_t::available)
        //     {
        //         this->cid_bitmap[i] = bitmap_t::used;
        //         next_cid = i;
        //         break;
        //     }
        // }

        // TODO ?
        size_t next_cid = this->cid++;
        cmd.common.commandId = next_cid;

        this->submission.submit_command(cmd);
        if (this->await() == false)
            return false;
        auto ret = this->completion.next_cmd_result();
        // this->cid_bitmap[next_cid] = bitmap_t::available;
        return ret.has_value();
    }

    bool QueuePair::submit_command(spec::Command cmd)
    {
        std::unique_lock guard(this->lock);
        return this->submit_command_nolock(cmd);
    }

    bool QueuePair::await()
    {
        return this->simple_event.await_timeout(3000);
        // this->event.await();
    }

    void QueuePair::trigger()
    {
        this->simple_event.trigger(false);
        // this->event.trigger(false);
    }

    bool Namespace::read_sectors(uint64_t sector, size_t num, void *buffer)
    {
        return this->rw_lba(spec::CommandOpcode::read, sector, num, buffer);
    }

    bool Namespace::write_sectors(uint64_t sector, size_t num, const void *buffer)
    {
        return this->rw_lba(spec::CommandOpcode::write, sector, num, const_cast<void*>(buffer));
    }

    bool Namespace::rw_lba(spec::CommandOpcode opcode, size_t sector, size_t num, void *buffer)
    {
        assert(num > 0);

        spec::Command cmd
        {
            .readWrite {
                .opcode = static_cast<uint8_t>(opcode),
                .flags = 0,
                .commandId = 0,
                .nsid = this->nsid,
                .rsv2 = 0,
                .metadata = 0,
                .dataPtr {
                    .prp1 = fromhh(reinterpret_cast<uintptr_t>(buffer)),
                    .prp2 = 0
                },
                .startLba = sector,
                .length = static_cast<uint16_t>(num - 1),
                .control = 0,
                .dsMgmt = 0,
                .refTag = 0,
                .appTag = 0,
                .appMask = 0
            }
        };

        auto ret = this->controller->allocate_ioqueue();
        if (ret.has_value() == false)
            return false;

        auto &queue = ret.value().get();

        auto size = this->block_size * num;
        if (size > pmm::page_size)
        {
            size_t prp_num = ((num - 1) * this->block_size) / pmm::page_size;
            if (prp_num > this->max_prps)
                return false;

            auto prp_list = std::allocate_unique<uint64_t[]>(this->prp_alloc, this->max_prps);
            auto vprp_list = tohh(prp_list.get());

            for (size_t i = 0; i < prp_num; i++)
                vprp_list[i] = fromhh(reinterpret_cast<uintptr_t>(buffer)) + pmm::page_size + i * pmm::page_size;

            cmd.readWrite.dataPtr.prp2 = reinterpret_cast<uintptr_t>(prp_list.get());
        }

        return queue->submit_command_nolock(cmd);
    }

    std::optional<std::reference_wrapper<std::unique_ptr<QueuePair>>> Controller::allocate_ioqueue()
    {
        for (auto &queue : this->io_queues)
        {
            if (queue->lock.try_lock())
                return queue;
        }
        return std::nullopt;
    }

    using expected_void = std::expected<void, const char *>;
    expected_void Controller::init()
    {
        this->dev->command(pci::CMD_BUS_MAST | pci::CMD_MEM_SPACE, true);

        auto bar0 = dev->getbars()[0];
        if (bar0.type != pci::PCI_BARTYPE_MMIO)
            return std::unexpected("BAR0 is not MMIO");

        this->regs = reinterpret_cast<Registers*>(bar0.map(vmm::gib1 * 2));

        uint32_t mjr = this->regs->version.mjr;
        uint32_t mnr = this->regs->version.mnr;
        uint32_t ter = this->regs->version.ter;
        log::infoln("NVME: Version {}.{}.{}", mjr, mnr, ter);

        auto css = this->regs->caps.css;
        if ((css & 0x01) == 0)
            return std::unexpected("NVM command set not supported");

        if (this->regs->toggle(false) == false)
            return std::unexpected("Could not disable controller");

        auto queue_size = this->regs->caps.mqes;
        {
            auto [handler, vector] = interrupts::allocate_handler();
            this->admin_queue = std::make_unique<QueuePair>(this->queue_ids++, this->regs, queue_size, 0, handler);

            if (dev->msix_set(smp::bsp_id, vector, -1) == false)
                return std::unexpected("Could not install Admin Queue IRQ handler");

            handler.set([&](auto) { this->admin_queue->trigger(); });
        }

        this->regs->aqa.asqs = queue_size - 1;
        this->regs->aqa.acqs = queue_size - 1;

        this->regs->asq = this->admin_queue->submission.phys_queue;
        this->regs->acq = this->admin_queue->completion.phys_queue;

        this->regs->ctrlconf.css = 0b000; // NVM command set
        this->regs->ctrlconf.ams = 0b000; // Round robin
        this->regs->ctrlconf.iosqes = 6;
        this->regs->ctrlconf.iocqes = 4;

        if (this->regs->toggle(true) == false)
            return std::unexpected("Could not enable controller");

        // Identify Cotroller
        {
            std::physical_allocator<spec::IdentifyController> identify_allocator;
            auto identify = std::allocate_unique<spec::IdentifyController>(identify_allocator);

            spec::Command cmd
            {
                .identify {
                    .opcode = spec::AdminOpcode::identify,
                    .flags = 0,
                    .commandId = 0,
                    .nsid = 0,
                    .rsv2 { },
                    .dataPtr {
                        .prp1 = reinterpret_cast<uintptr_t>(identify.get()),
                        .prp2 = 0,
                    },
                    .cns = spec::IdentifyCNS::identifyController,
                    .rsv3 = 0,
                    .controllerId = 0,
                    .rsv11 { }
                }
            };

            if (this->admin_queue->submit_command(cmd) == false)
                return std::unexpected("Could not identify controller");

            this->identity = *tohh(identify.get());
            log::infoln("NVME: Identified controller: (Vendor ID: {}, Subsystem Vendor ID: {})", this->identity.vid, this->identity.ssvid);
        }

        auto nsid_list_pages = div_roundup(this->identity.nn * sizeof(uint32_t), pmm::page_size);
        auto nsid_list = pmm::alloc<uint32_t*>(nsid_list_pages);
        { // Identify Namespace
            spec::Command cmd
            {
                .identify {
                    .opcode = spec::AdminOpcode::identify,
                    .flags = 0,
                    .commandId = 0,
                    .nsid = 0,
                    .rsv2 { },
                    .dataPtr {
                        .prp1 = reinterpret_cast<uintptr_t>(nsid_list),
                        .prp2 = 0
                    },
                    .cns = spec::IdentifyCNS::identifyActiveList,
                    .rsv3 = 0,
                    .controllerId = 0,
                    .rsv11 { }
                }
            };

            if (this->admin_queue->submit_command(cmd) == false)
                return std::unexpected("could not get NSID list");
        }

        uint16_t irq = 1;
        for (size_t i = 0; i < num_io_queues; i++)
        {
            auto [handler, vector] = interrupts::allocate_handler();
            auto io_queue = std::make_unique<QueuePair>(this->queue_ids++, this->regs, queue_size, irq++, handler);

            if (dev->msix_set(smp::bsp_id, vector, -1) == false)
            {
                if (i == 0)
                    return std::unexpected("Could not install any IRQ handlers for IO queues.");

                log::warnln("NVME: Could not install IRQ handlers for some IO queues. {}/{}", i, num_io_queues);
                break; // I guess we will have fewer io queues
            }

            spec::Command io_cq_cmd
            {
                .createCQ {
                    .opcode = spec::AdminOpcode::createCQ,
                    .flags = 0,
                    .commandId = 0,
                    .rsv1 { },
                    .prp1 = io_queue->completion.phys_queue,
                    .prp2 = 0,
                    .cqid = io_queue->id,
                    .qSize = static_cast<uint16_t>(io_queue->size - 1),
                    .cqFlags = spec::queuePhysContig | spec::CQIrqEnabled,
                    .irqVector = io_queue->irq,
                    .rsv2 { }
                }
            };
            if (this->admin_queue->submit_command(io_cq_cmd) == false)
                return std::unexpected("Unable to create completion queue");

            spec::Command io_sq_cmd
            {
                .createSQ {
                    .opcode = spec::AdminOpcode::createSQ,
                    .flags = 0,
                    .commandId = 0,
                    .rsv1 { },
                    .prp1 = io_queue->submission.phys_queue,
                    .prp2 = 0,
                    .sqid = io_queue->id,
                    .qSize = static_cast<uint16_t>(io_queue->size - 1),
                    .sqFlags = spec::queuePhysContig | spec::CQIrqEnabled,
                    .cqid = io_queue->id,
                    .rsv2 { }
                }
            };
            if (this->admin_queue->submit_command(io_sq_cmd) == false)
                return std::unexpected("Unable to create submission queue");

            handler.set([ptr = io_queue.get()](auto) { ptr->trigger(); });

            log::infoln("NVME: Created IO Queue with ID: {}", io_queue->id);
            this->io_queues.emplace_back(std::move(io_queue));
        }

        size_t maxtransshift = 20;
        if (this->identity.mdts != 0)
            maxtransshift = 12 + this->regs->caps.mpsmin + this->identity.mdts;

        auto vnsid_list = tohh(nsid_list);
        for (uint32_t i = 0; i < this->identity.nn; i++)
        {
            auto nsid = vnsid_list[i];
            if (nsid != 0)
            {
                std::physical_allocator<spec::IdentifyNamespace> identify_allocator;
                auto identify = std::allocate_unique<spec::IdentifyNamespace>(identify_allocator);

                spec::Command cmd
                {
                    .identify {
                        .opcode = spec::AdminOpcode::identify,
                        .flags = 0,
                        .commandId = 0,
                        .nsid = nsid,
                        .rsv2 { },
                        .dataPtr {
                            .prp1 = reinterpret_cast<uintptr_t>(identify.get()),
                            .prp2 = 0
                        },
                        .cns = spec::IdentifyCNS::identifyNamespace,
                        .rsv3 = 0,
                        .controllerId = 0,
                        .rsv11 { }
                    }
                };
                if (this->admin_queue->submit_command(cmd) == false)
                {
                    log::errorln("NVME: Unable to identify namespace 0x{:X}", nsid);
                    continue;
                }

                auto nsidentity = tohh(identify.get());

                size_t blocks = nsidentity->nsze;
                size_t block_size = 1 << nsidentity->lbaf[nsidentity->flbas & 0b11111].ds;

                size_t lbashift = nsidentity->lbaf[nsidentity->flbas & 0x0F].ds;
                if (lbashift == 0)
                    lbashift = 9;

                size_t max_lbas = 1 << (maxtransshift - lbashift);
                size_t max_prps = (max_lbas * (1 << lbashift)) / pmm::page_size;

                log::infoln("NVME: Identified namespace 0x{:X} with size: {}mib", nsid, blocks * block_size / 1024 / 1024);
                this->namespaes.push_back(std::make_unique<Namespace>(this, nsid, blocks, block_size, max_prps));
            }
        }
        pmm::free(nsid_list, nsid_list_pages);

        return this->namespaes.empty() ? std::unexpected("No usable namespaces") : expected_void();
    }

    Controller::~Controller()
    {
        if (this->admin_queue)
            this->admin_queue->handler.reset();

        for (auto &io_queue : this->io_queues)
            io_queue->handler.reset();
    }
} // namespace nvme

DRIVER(nvme, init, fini)

__init__ bool init()
{
    bool at_least_one = false;
    for (const auto dev : pci::get_devices())
    {
        if (dev->Class != nvme::Class || dev->subclass != nvme::subclass || dev->progif != nvme::progif)
            continue;

        log::infoln("NVME: Found controller");

        auto ctrl = std::make_unique<nvme::Controller>(dev);
        auto ret = ctrl->init();
        if (ret.has_value())
        {
            at_least_one = true;
            nvme::ctrls.push_back(std::move(ctrl));
        }
        else log::errorln("NVME: {}!", ret.error());
    }

    return at_least_one;
}

__fini__ bool fini()
{
    assert(false, "NVME->fini() not implemented!");
    return false;
}