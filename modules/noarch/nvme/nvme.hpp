// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <drivers/pci/pci.hpp>
#include <lib/interrupts.hpp>
#include <lib/event.hpp>
#include <mm/pmm.hpp>

#include <expected>
#include <cstdint>

#include "spec.hpp"

namespace nvme
{
    constexpr inline uint8_t Class = 0x01;
    constexpr inline uint8_t subclass = 0x08;
    constexpr inline uint8_t progif = 0x02;

    constexpr inline size_t num_io_queues = 4;

    struct Registers
    {
        volatile struct [[gnu::packed]]
        {
            volatile struct [[gnu::packed]]
            {
                uint64_t mqes : 16;
                uint64_t cqr : 1;
                uint64_t ams : 2;
                uint64_t rsv0 : 5;
                uint64_t to : 8;
                uint64_t dstrd : 4;
                uint64_t nssrs : 1;
                uint64_t css : 8;
                uint64_t bps : 1;
                uint64_t cps : 2;
                uint64_t mpsmin : 4;
                uint64_t mpsmax : 4;
                uint64_t pmrs : 1;
                uint64_t cmbs : 1;
                uint64_t nsss : 1;
                uint64_t crms : 2;
                uint64_t rsv1 : 3;
            } caps;
            volatile struct [[gnu::packed]]
            {
                uint32_t ter : 8;
                uint32_t mnr : 8;
                uint32_t mjr : 16;
            } version;
            uint32_t intms;
            uint32_t intmc;
            volatile struct [[gnu::packed]]
            {
                uint32_t en : 1;
                uint32_t rsv0 : 3;
                uint32_t css : 3;
                uint32_t mps : 4;
                uint32_t ams : 3;
                uint32_t shn : 2;
                uint32_t iosqes : 4;
                uint32_t iocqes : 4;
                uint32_t crime : 1;
                uint32_t rsv1 : 7;
            } ctrlconf;
            uint32_t rsv0;
            volatile struct [[gnu::packed]]
            {
                uint32_t rdy : 1;
                uint32_t cfs : 1;
                uint32_t shst : 2;
                uint32_t nssro : 1;
                uint32_t pp : 1;
                uint32_t st : 1;
                uint32_t rsv0 : 25;
            } ctrlstat;
            uint32_t nssr;
            volatile struct [[gnu::packed]]
            {
                uint32_t asqs : 12;
                uint32_t rsv0 : 4;
                uint32_t acqs : 12;
                uint32_t rsv1 : 4;
            } aqa;
            uint64_t asq;
            uint64_t acq;
        };

        bool toggle(bool enable);
    };

    template<typename Der, typename Type>
    struct Queue
    {
        static inline size_t multiplier;
        static inline size_t getdboffset(uint32_t qid, uint64_t strides)
        {
            return 0x1000 + (qid * 2 + multiplier) * (4 << strides);
        }

        Queue(Registers *regs, uint16_t id, size_t size);
        ~Queue();

        volatile Type *queue;
        const size_t queue_size;

        uintptr_t phys_queue;
        std::physical_allocator<Type> phys_allocator;

        size_t index;
        volatile uint32_t *doorbell;
    };

    struct SubmissionQueue : Queue<SubmissionQueue, spec::Command>
    {
        using Queue<SubmissionQueue, spec::Command>::Queue;
        void submit_command(spec::Command cmd);
    };
    template<> inline size_t Queue<SubmissionQueue, spec::Command>::multiplier = 0;

    struct CompletionQueue : Queue<CompletionQueue, spec::CompletionEntry>
    {
        using Queue<CompletionQueue, spec::CompletionEntry>::Queue;
        std::expected<spec::CompletionEntry, uint16_t> next_cmd_result();

        bool phase = true;
    };
    template<> inline size_t Queue<CompletionQueue, spec::CompletionEntry>::multiplier = 1;

    struct QueuePair
    {
        event::simple::event_t simple_event;
        // event::event_t event;
        std::mutex lock;

        // bitmap_t cid_bitmap;
        uint16_t cid;
        uint16_t id;
        uint16_t irq;

        interrupts::handler &handler;

        size_t size;
        SubmissionQueue submission;
        CompletionQueue completion;

        QueuePair(uint16_t qid, Registers *regs, size_t size, uint16_t irq, interrupts::handler &handler) : /* cid_bitmap(size), */ cid(0),
            id(qid), irq(irq), handler(handler), size(size),
            submission(regs, id, size), completion(regs, id, size) { }

        bool submit_command_nolock(spec::Command cmd);
        bool submit_command(spec::Command cmd);

        bool await();
        void trigger();
    };

    struct Controller;

    // TODO: Base block device class
    struct Namespace
    {
        Controller *controller;
        uint32_t nsid;
        size_t blocks;
        size_t block_size;
        size_t max_prps;

        Namespace(Controller *controller, uint32_t nsid, size_t blocks, size_t block_size, size_t max_prps) :
            controller(controller), nsid(nsid), blocks(blocks), block_size(block_size), max_prps(max_prps), prp_alloc() { }

        bool read_sectors(size_t sector, size_t num, void *buffer);
        bool write_sectors(size_t sector, size_t num, const void *buffer);

        private:
        std::physical_allocator<uint64_t> prp_alloc;

        bool rw_lba(spec::CommandOpcode opcode, size_t sector, size_t num, void *buffer);
    };

    struct Controller
    {
        pci::device_t *dev;
        Registers *regs;

        spec::IdentifyController identity;
        std::vector<std::unique_ptr<Namespace>> namespaes;

        uint64_t queue_ids;
        std::unique_ptr<QueuePair> admin_queue;
        std::vector<std::unique_ptr<QueuePair>> io_queues;

        Controller(pci::device_t *dev) : dev(dev), identity(), namespaes(), queue_ids(0), io_queues() { }
        ~Controller();

        std::optional<std::reference_wrapper<std::unique_ptr<QueuePair>>> allocate_ioqueue();
        std::expected<void, const char *> init();
    };
} // namespace nvme