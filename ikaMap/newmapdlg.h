#ifndef NEWMAPDLG_H
#define NEWMAPDLG_H

#include "wxinc.h"
#include <string>

class NewMapDlg : public wxDialog
{
    template <class T>
        T* Ctrl(const std::string& name, const T* = 0);

    void OnOK(wxCommandEvent& event);
    void OnBrowse(wxCommandEvent& event);
    void OnCheckNewTileSet(wxCommandEvent& event);
public:
    NewMapDlg(wxWindow* parent);

    // too lazy to encapsulate this shit.
    int width;
    int height;
    std::string tileSetName;
    bool newTileSet;
    int tileWidth;
    int tileHeight;

    DECLARE_EVENT_TABLE()
};

#endif