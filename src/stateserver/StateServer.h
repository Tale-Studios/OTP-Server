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

  protected:
    std::unique_ptr<LogCategory> m_log;
    std::unordered_map<doid_t, DistributedObject*> m_objs;
    std::unordered_map<doid_t, uint16_t> m_dcids;
    doid_t m_root_object = 0;

  private:
    void handle_generate(DatagramIterator &dgi, bool has_other);
    void handle_delete_ai(DatagramIterator &dgi, channel_t sender);
};
