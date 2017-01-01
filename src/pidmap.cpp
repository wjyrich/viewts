#include "pidmap.h"
#include "packet.h"
#include "pes.h"

pidmap::pidmap(std::ifstream& fileIn) :
    m_fileIn(fileIn), m_prev_pid(-1), m_prev_pattern(255)
{
    // align file on first 0x47
    char start[512];
    int index = 0;

    m_fileIn.read((char*)start, 512);
    while (start[index] != 0x47 && start[index+188] != 0x47 && (index+188) < 512) index++;

    // loop on packet
    m_fileIn.clear();
    m_fileIn.seekg(index);
}

pidmap::~pidmap()
{
    if (!m_pidMap.empty()) {
        m_pidMap.clear();
    }
    if (!m_pidVec.empty()) {
        m_pidVec.clear();
    }
}

// Cpu consuming function
bool pidmap::run(unsigned int nbPacketToRead)
{
    unsigned char data[188];
    bool isDataToRead = false;

    while (nbPacketToRead)
    {
        // leave if no more data
        if (! m_fileIn.read((char*)data, 188)) break;
        isDataToRead = true;

        // create packet from buffer
        packet packet(data);
        unsigned int pid = packet.getPid();

        // update pid list
        m_pidVec.push_back(pid);

        // update pattern
        if (m_pidMap[pid].nb_packet == 0)
            m_pidMap[pid].pattern = (char)('0' + m_pidMap.size() - 1);

        // other values
        m_pidMap[pid].nb_packet++;
        m_pidMap[pid].has_pcr |= packet.hasPcr();

        if (packet.hasPes()) {
            // create pes from buffer
            pes pes(data + packet.getPesOffset());
            m_pidMap[pid].has_pts |= pes.hasPts();
            m_pidMap[pid].has_dts |= pes.hasDts();
        }
    }

    // calc percentage
    std::map<unsigned int, pidinfo>::iterator ii;
    for (ii=m_pidMap.begin(); ii!=m_pidMap.end(); ++ii)
    {
        (*ii).second.percent = static_cast<float>((*ii).second.nb_packet)*100;
        (*ii).second.percent /= m_pidVec.size();
    }

    // return true if some data are uptated
    return isDataToRead;
}

void pidmap::getPcrPid(std::vector<unsigned int>&  pidVector) {

    std::map<unsigned int, pidinfo>::iterator ii;
    for (ii=m_pidMap.begin(); ii!=m_pidMap.end(); ++ii)
    {
        if((*ii).second.has_pcr)
            pidVector.push_back((*ii).first);
    }
}

void pidmap::getPtsPid(std::vector<unsigned int>&  pidVector) {

    std::map<unsigned int, pidinfo>::iterator ii;
    for (ii=m_pidMap.begin(); ii!=m_pidMap.end(); ++ii)
    {
        if((*ii).second.has_pts)
            pidVector.push_back((*ii).first);
    }
}

void pidmap::getDtsPid(std::vector<unsigned int>&  pidVector) {

    std::map<unsigned int, pidinfo>::iterator ii;
    for (ii=m_pidMap.begin(); ii!=m_pidMap.end(); ++ii)
    {
        if((*ii).second.has_dts)
            pidVector.push_back((*ii).first);
    }
}

bool pidmap::GetNextPidInfo(unsigned int& pid, pidinfo& pidInfo)
{
    // protection
    if (m_pidMap.empty())
        return false;

    // init iterator
    if (m_prev_pid == -1) {

        m_pidMap_ii = m_pidMap.begin();
        m_prev_pid = pid = (*m_pidMap_ii).first;
        pidInfo = (*m_pidMap_ii).second;
        return true;
    }

    if (m_pidMap_ii != --m_pidMap.end()) {

        ++m_pidMap_ii;
        m_prev_pid = pid = (*m_pidMap_ii).first;
        pidInfo = (*m_pidMap_ii).second;
        return true;
    }

    return false;
}

bool pidmap::GetNextPattern(unsigned char& pattern)
{
    // protection
    if (m_pidVec.empty() || m_pidMap.empty())
        return false;

    // init iterator
    if (m_prev_pattern == 255) {

        m_pidVec_ii = m_pidVec.begin();
        m_prev_pattern = pattern = m_pidMap[*m_pidVec_ii].pattern;
        return true;
    }

    if (m_pidVec_ii != --m_pidVec.end()) {

        ++m_pidVec_ii;
        m_prev_pattern = pattern = m_pidMap[*m_pidVec_ii].pattern;
        return true;
    }

    return false;
}
