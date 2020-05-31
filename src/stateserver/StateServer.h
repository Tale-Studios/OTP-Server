#pragma once
#include <memory>
#include <unordered_map>
#include "core/Role.h"
#include "core/RoleFactory.h"

class DistributedObject;

class StateServer : public Role
{
    friend class DistributedObject;

  public:
    StateServer(RoleConfig roleconfig);

    virtual void handle_datagram(DatagramHandle in_dg, DatagramIterator &dgi);

    // get_dcid returns the DC ID of an object by its DO ID.
    inline uint16_t get_dcid(doid_t do_id)
    {
        for(auto it : m_dcids) {
            if(it.first == do_id) {
                return it.second;
            }
        }

        return 0;
    }

    // free_id frees an already-allocated ID.
    void free_id(doid_t id);

  protected:
    std::unique_ptr<LogCategory> m_log;
    std::unordered_map<doid_t, DistributedObject*> m_objs;
    std::unordered_map<doid_t, uint16_t> m_dcids;
    channel_t m_channel;
    doid_t m_root_object;

  private:
    void handle_generate(DatagramIterator &dgi, channel_t sender, bool has_other);
    void handle_delete_ai(DatagramIterator &dgi, channel_t sender);

    // get_next_id returns the next available ID to be used in object creation.
    doid_t get_next_id();

    // Variables for object ID allocation.
    doid_t m_min_id, m_max_id, m_next_free, m_last_free, *m_table;
};
