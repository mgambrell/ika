#include "configfile.h"
#include "fileio.h"

CConfigFile::CConfigFile(const char* fname)
{
    Read(fname);
}

void CConfigFile::Add(const std::string& key,const std::string& value)
{
    keys[key]=value;
}

std::string CConfigFile::Get(const std::string& key)
{
    ConfigMap::iterator i=keys.find(key);

    if (i==keys.end())
        return "";

    return (*i).second;
}

int CConfigFile::GetInt(const std::string& key)
{
    return atoi(Get(key).c_str());
}

void CConfigFile::Read(const char* fname)
{
    File f;

    bool result=f.OpenRead(fname);
    if (!result)    return;

    char key[1024],value[1024];
    while (!f.eof())
    {
        f.ReadToken(key);
        f.ReadToken(value);

        // empty keys/values are no good
        if (!key[0] || !value[0])
            return;

        Add(key,value);
    }

    f.Close();
}

void CConfigFile::Write(const char* fname)
{
    File f;

    f.OpenWrite(fname);

    for (ConfigMap::iterator i=keys.begin(); i!=keys.end(); i++)
    {
        f.Write(i->first.c_str());
        f.Write(" ");
        f.Write(i->second.c_str());
        f.Write("\n");
    }

    f.Close();
}