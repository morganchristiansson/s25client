// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class glFont;
struct Inventory;
class GamePlayer;
class ctrlTab;
class glArchivItem_Bitmap;

class iwWares : public IngameWindow
{
protected:
    const Inventory& inventory; /// Warenbestand
    const GamePlayer& player;
    unsigned warePageID, peoplePageID;
    ctrlTab* tabCtrl;

public:
    iwWares(unsigned id, const DrawPoint& pos, unsigned additionalYSpace, const std::string& title,
            bool allow_outhousing, const glFont* font, const Inventory& inventory, const GamePlayer& player);

protected:
    /// bestimmte Inventurseite zeigen.
    virtual void SetPage(unsigned page);
    /// Add a new tab page and return it. @p tabId is set to the tab's ID (for use with GetCurrentTab()).
    ctrlGroup& AddPage(glArchivItem_Bitmap* image, const std::string& tooltip, unsigned& tabId);

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_PaintBefore() override;
    void Msg_TabChange(unsigned ctrl_id, unsigned short tab_id) override;

    unsigned GetCurPage() const { return curPage_; }

private:
    unsigned curPage_; /// aktuelle Seite des Inventurfensters.
    unsigned nextTabId; /// nächstes Tab-ID
    static constexpr unsigned ID_pageOffset = 100;
};
