//
//  DiskMultiMap.cpp
//  pj4
//
//  Created by Binyi Wu on 3/5/16.
//  Copyright © 2016 Binyi Wu. All rights reserved.
//

#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include <iostream>
#include <functional>
using namespace std;

DiskMultiMap::DiskMultiMap(){}
DiskMultiMap::~DiskMultiMap()
{
    if (m_file.isOpen())
        close();
}

bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets)
{
    if (m_file.isOpen())
        m_file.close();
    
    m_file.createNew(m_free);
    m_file.write(head(0,0,0),0);   //head structrue for freed space 0 used, 0 in total
    m_file.close();
    
    if (!m_file.createNew(filename))
        return false;
    m_fn = filename;
    m_bucket = numBuckets;
    m_file.write(head(0,0,0),0);   //head structure for DiskMultiMap  0 used, 0 in total
    for (int i=0; i<numBuckets; i++)
    {
        Bucket bk;
        bk.next = 0;
        bk.used = false;
        m_file.write(bk,m_file.fileLength());
        
    }
    int n = m_file.fileLength();
    m_file.close();
    return true;
}

bool DiskMultiMap::openExisting(const std::string &filename)
{
    if (!m_file.openExisting(filename))
        return false;
    return true;
}

void DiskMultiMap::close()
{
    if (m_file.isOpen())
        m_file.close();
}
bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{
    if (key.size() > 120 || value.size() > 120 || context.size() > 120)
        return false;
    if (!m_file.openExisting(m_fn))
        return false;
    BinaryFile::Offset prev = 0;
    BinaryFile::Offset Offset = keyhash(key);
    BinaryFile::Offset start = 0;
    int k = m_file.fileLength();
    
    m_file.close();
    
    
    m_file.openExisting(m_free);
    head h = head(0,0,0);
    m_file.read(h,0);
    if (h.numinuse != 0)
    {   if (h.next==0)
            cout<<"Error, wrong link";
        start = h.next;
        m_file.write(head(h.numinuse-1,h.numintotal,h.next),0);
        freememory t = freememory(0,0);
        m_file.read(t,h.next);
        m_file.write(freememory(0,t.next),start);
       
    }
    if (start == 0 )
        start = k;
    m_file.close();
    
    
    m_file.openExisting(m_fn);
    Bucket bk;
    m_file.read(bk,Offset);
    if (bk.used == false)
    {
        strcpy(bk.key,key.c_str());
        bk.next = start;
        bk.used = true;
        m_file.write(bk,Offset);
        m_file.write(DiskNode(key,value,context,0),start);
    }
    else
    {   prev = bk.next;
        bk.next = start;
        m_file.write(bk,Offset);
        m_file.write(DiskNode(key,value,context,prev),start);
    }
    
    
//for test purpose///////////////
    DiskNode d;
    m_file.read(d,start);
//need to comment out it/////////////
  
    m_file.close();
    return true;
}

BinaryFile::Offset DiskMultiMap::keyhash(string key)
{
    hash<string> hasher;
    unsigned int rkey = hasher(key);
    Bucket bk(key);
    head a = head(0,0,0);
    rkey = (((rkey % (m_bucket))*sizeof(bk))+sizeof(a));
    return rkey;
}
int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
    if (!m_file.openExisting(m_fn))
        return -1;
    int num = 0;
    BinaryFile::Offset Offset = keyhash(key);
    Bucket b;
    if (m_file.read(b,Offset) && b.used == false)
        return 0;
    
    DiskNode current;
    m_file.read(current,b.next);
    BinaryFile::Offset os = b.next;
    BinaryFile::Offset prev = Offset;
    for(;;)
    {
        if (strcmp(current.key,key.c_str())!=0 || strcmp(current.value,value.c_str())!=0 || strcmp(current.context,context.c_str())!=0)
        {
            if (current.next == 0)
                return num;
            prev = os;
            os = current.next;
            m_file.read(current,current.next);
            continue;
        }
        
        else
        {   num++;
            m_file.close();
            if (!m_file.openExisting(m_free))
                return -1;
            head h = head(0,0,0);
            BinaryFile::Offset pos = 0;
            if (m_file.read(h,0)&&h.numinuse!=h.numintotal)
            {   pos = h.next;
                freememory f = freememory(0,0);
                m_file.read(f,f.next);
                while(f.num ==0)
                {
                    pos = f.next;
                    m_file.read(f,f.next);
                }
                m_file.read(f,pos);
                m_file.write(freememory(os,f.next),pos);
                m_file.write(head(h.numinuse+1,h.numintotal,h.next),0);
                m_file.close();
                
            }
            else
            {
                m_file.write(freememory(os,0),m_file.fileLength());
                m_file.write(head(h.numinuse+1,h.numintotal+1,h.next),0);
                m_file.close();
            }
        }
        m_file.openExisting(m_fn);
        DiskNode pv;
        m_file.read(pv,prev);
        m_file.write(DiskNode(pv.key,pv.value,pv.context,current.next),prev);
        if (current.next == 0)
            break;
        else
            m_file.read(current,current.next);
        }
    
    m_file.close();
    return num;
    
    
}

DiskMultiMap::Iterator::Iterator()
{
    
}
