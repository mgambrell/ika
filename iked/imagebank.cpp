#include "imagebank.h"
#include "graph.h"

CImageBank::~CImageBank()
{
    FreeBitmaps();
}

void CImageBank::Sync()
{
    for (std::set<int>::iterator i=altered.begin(); i!=altered.end(); i++)
    {
        bitmaps[*i]->Update(Get(*i));
    }

    altered.clear();
}

void CImageBank::SyncAll()
{
    FreeBitmaps();

    bitmaps.resize(Count());

    for (int i=0; i<Count(); i++)
    {
        bitmaps[i]=new CImage(Get(i));
    }
}

void CImageBank::FreeBitmaps()
{
    for (int i=0; i<bitmaps.size(); i++)
    {
        delete bitmaps[i];
    }
    bitmaps.clear();
}

void CImageBank::Set(CPixelMatrix& img,int idx)
{
    if (idx<0 || idx>=Count())
        return;

    SetImage(img,idx);
    altered.insert(idx);
}

CImage& CImageBank::GetImage(int idx)
{
    if (altered.find(idx)!=altered.end())   // has this image been altered?
    {
        bitmaps[idx]->Update(Get(idx));     // sync it
        altered.erase(idx);
    }

    if (idx<0 || idx>=Count())
        return *bitmaps[0];

    return *bitmaps[idx];
}