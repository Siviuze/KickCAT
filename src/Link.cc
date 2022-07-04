#include "Link.h"
#include "AbstractSocket.h"
#include "Time.h"

namespace kickcat
{
    Link::Link(std::shared_ptr<AbstractSocket> socketNominal, std::shared_ptr<AbstractSocket> socketRedundancy)
        : socketNominal_(socketNominal)
        , socketRedundancy_(socketRedundancy)
    {

    }


    void Link::writeThenRead(Frame& frame)
    {
        frame.write(socketNominal_, socketRedundancy_);

        try
        {
            frame.read(socketRedundancy_);
            auto [header, _, wkc] = frame.nextDatagram();
            printf("Nominal frame wkc %i \n", wkc);
        }
        catch (std::exception const& e)
        {
            DEBUG_PRINT("%s\n", e.what());
        }

        try
        {
            Frame debugFrame;
            debugFrame.read(socketNominal_);
            auto [header, _, wkc] = debugFrame.nextDatagram();
            printf("Redundancy frame wkc %i Datagrame size %i \n", wkc, header->len);
        }
        catch (std::exception const& e)
        {
            DEBUG_PRINT("%s\n", e.what());
        }
    }


    void Link::sendFrame()
    {
        // save number of datagrams in the frame to handle send error properly if any
        int32_t const datagrams = frame_.datagramCounter();

        try
        {
            frame_.write(socketNominal_);
            ++sent_frameNominal_;
        }
        catch (std::exception const& e)
        {
            DEBUG_PRINT("%s\n", e.what());

            for (int32_t i = 0; i < datagrams; ++i)
            {
                int32_t index = index_head_ - i - 1;
                callbacks_[index].status = DatagramState::SEND_ERROR;
            }
        }
    }


    void Link::addDatagram(enum Command command, uint32_t address, void const* data, uint16_t data_size,
                           std::function<DatagramState(DatagramHeader const*, uint8_t const* data, uint16_t wkc)> const& process,
                           std::function<void(DatagramState const& state)> const& error)
    {
        if (index_queue_ == static_cast<uint8_t>(index_head_ + 1))
        {
            THROW_ERROR("Too many datagrams in flight. Max is 255");
        }

        uint16_t const needed_space = datagram_size(data_size);
        if (frame_.freeSpace() < needed_space)
        {
            sendFrame();
        }

        frame_.addDatagram(index_head_, command, address, data, data_size);
        callbacks_[index_head_].process = process;
        callbacks_[index_head_].error = error;
        callbacks_[index_head_].status = DatagramState::LOST;
        ++index_head_;

        if (frame_.isFull())
        {
            sendFrame();
        }
    }


    void Link::finalizeDatagrams()
    {
        if (frame_.datagramCounter() != 0)
        {
            sendFrame();
        }
    }


    void Link::processDatagrams()
    {
        finalizeDatagrams();

        uint8_t waiting_frame = sent_frameNominal_;
        sent_frameNominal_ = 0;

        for (int32_t i = 0; i < waiting_frame; ++i)
        {
            try
            {
                frame_.read(socketRedundancy_);
                while (frame_.isDatagramAvailable())
                {
                    auto [header, data, wkc] = frame_.nextDatagram();
                    callbacks_[header->index].status = callbacks_[header->index].process(header, data, wkc);
                }
            }
            catch (std::exception const& e)
            {
                DEBUG_PRINT("%s\n", e.what());
            }
        }

        std::exception_ptr client_exception;
        for (uint8_t i = index_queue_; i != index_head_; ++i)
        {
            if (callbacks_[i].status != DatagramState::OK)
            {
                // Datagram was either lost or processing it encountered an error.
                try
                {
                    callbacks_[i].error(callbacks_[i].status);
                }
                catch (...)
                {
                    client_exception = std::current_exception();
                }
            }

            // Attach a callback to handle not THAT lost frames.
            // -> if a frame suspected to be lost was in fact in the pipe, it is needed to pop it
            callbacks_[i].process = [&](DatagramHeader const*, uint8_t const*, uint16_t){ frame_.read(socketRedundancy_); return DatagramState::OK; };
        }

        index_queue_ = index_head_;
        frame_.clear();

        // Rethrow last catched client exception.
        if (client_exception)
        {
            std::rethrow_exception(client_exception);
        }
    }
}
