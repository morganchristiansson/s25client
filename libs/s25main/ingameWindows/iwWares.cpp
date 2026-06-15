// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwWares.h"
#include "AddonHelperFunctions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "WindowManager.h"
#include "WineLoader.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlTab.h"
#include "controls/ctrlText.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "world/GameWorld.h"
#include "gameData/GoodConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

namespace {
constexpr unsigned TAB_HEIGHT = 45;
constexpr unsigned rowHeight = 42;
constexpr unsigned topMargin = 21;
constexpr unsigned bottomMargin = 11;
constexpr unsigned buttonRowHeight = 34;
constexpr unsigned spacingBetweenLastRowAndButtonRow = 16;
} // namespace

static void addElement(ctrlGroup& page, const glFont* font, const DrawPoint btPos, const Extent btSize,
                       const unsigned idOffset, const std::string& name, ITexture* img, const bool allow_outhousing)
{
    // Background image, only a button when outhousing is allowed
    if(allow_outhousing)
    {
        ctrlButton* b =
          page.AddImageButton(100 + idOffset, btPos, btSize, TextureColor::Grey, LOADER.GetMapTexture(2298), name);
        b->SetBorder(false);
    } else
        page.AddImage(100 + idOffset, btPos + btSize / 2, LOADER.GetMapTexture(2298), name);

    // Background image for the amount
    const DrawPoint bgCtPos = btPos + DrawPoint(btSize.x / 2, 32);
    page.AddImage(200 + idOffset, bgCtPos, LOADER.GetMapTexture(2299));

    // The actual image for the element
    const DrawPoint warePos = btPos + btSize / 2;
    page.AddImage(300 + idOffset, warePos, img);

    // Overlay for "don't collect"
    DrawPoint overlayPos = warePos - DrawPoint(0, 4);
    ctrlImage* image = page.AddImage(400 + idOffset, overlayPos, LOADER.GetImageN("io", 222));
    image->SetVisible(false);

    // Overlay for "send out"
    overlayPos = warePos + DrawPoint(0, 10);
    image = page.AddImage(500 + idOffset, overlayPos, LOADER.GetImageN("io", 221));
    image->SetVisible(false);

    // Overlay for "collect"
    image = page.AddImage(700 + idOffset, overlayPos, LOADER.GetImageN("io_new", 3));
    image->SetVisible(false);

    // Amount of the element
    const DrawPoint txtPos = btPos + DrawPoint(btSize.x, 40);
    page.AddText(600 + idOffset, txtPos, "", COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, font);
}

iwWares::iwWares(unsigned id, const DrawPoint& pos, unsigned additionalYSpace, const std::string& title,
                 bool allow_outhousing, const glFont* font, const Inventory& inventory, const GamePlayer& player)
    : IngameWindow(id, pos, Extent(167, 416), title, LOADER.GetImageN("io", 5)), inventory(inventory), player(player),
      nextTabId(ID_pageOffset)
{
    if(!font)
        font = SmallFont;

    // Zuordnungs-IDs
    std::vector<GoodType> WARE_DISPLAY_ORDER{
      GoodType::Wood,    GoodType::Boards,   GoodType::Stones,
      GoodType::Ham,     GoodType::Grain,    GoodType::Flour,
      GoodType::Fish,    GoodType::Meat,     GoodType::Bread,
      GoodType::Water,   GoodType::Beer,     GoodType::Coal,
      GoodType::IronOre, GoodType::Gold,     GoodType::Iron,
      GoodType::Coins,   GoodType::Tongs,    GoodType::Axe,
      GoodType::Saw,     GoodType::PickAxe,  GoodType::Hammer,
      GoodType::Shovel,  GoodType::Crucible, GoodType::RodAndLine,
      GoodType::Scythe,  GoodType::Cleaver,  GoodType::Rollingpin,
      GoodType::Bow,     GoodType::Sword,    GoodType::ShieldRomans /* nation specific */,
      GoodType::Boat,    GoodType::Grapes,   GoodType::Wine,
      GoodType::Skins,   GoodType::Leather,  GoodType::Armor,
    };

    std::vector<Job> JOB_DISPLAY_ORDER{Job::Helper,        Job::Builder,
                                       Job::Planer,        Job::Woodcutter,
                                       Job::Forester,      Job::Stonemason,
                                       Job::Fisher,        Job::Hunter,
                                       Job::Carpenter,     Job::Farmer,
                                       Job::PigBreeder,    Job::DonkeyBreeder,
                                       Job::Miller,        Job::Baker,
                                       Job::Butcher,       Job::Brewer,
                                       Job::Miner,         Job::IronFounder,
                                       Job::Armorer,       Job::Minter,
                                       Job::Metalworker,   Job::Shipwright,
                                       Job::Geologist,     Job::Scout,
                                       Job::PackDonkey,    Job::CharBurner,
                                       Job::Winegrower,    Job::Vintner,
                                       Job::TempleServant, Job::Skinner,
                                       Job::Tanner,        Job::LeatherWorker,
                                       Job::Private,       Job::PrivateFirstClass,
                                       Job::Sergeant,      Job::Officer,
                                       Job::General};

    helpers::erase_if(WARE_DISPLAY_ORDER, makeIsUnusedWare(player.GetGameWorld().GetGGS()));
    helpers::erase_if(JOB_DISPLAY_ORDER, makeIsUnusedJob(player.GetGameWorld().GetGGS()));

    // Tab-Control innerhalb des Inhaltsbereichs platzieren (neben Titelleiste)
    tabCtrl = AddTabCtrl(0, DrawPoint(contentOffset.x, contentOffset.y), GetIwSize().x);

    // Warenseite als Tab hinzufügen
    ctrlGroup* waresGroup = tabCtrl->AddTab(LOADER.GetImageN("io", 170), _("Goods"), nextTabId);
    warePageID = nextTabId++;
    // Figurenseite als Tab hinzufügen
    ctrlGroup* figuresGroup = tabCtrl->AddTab(LOADER.GetImageN("io", 169), _("People"), nextTabId);
    peoplePageID = nextTabId++;

    bool isRowWithFourElemens = true;
    const unsigned numElements = std::max(WARE_DISPLAY_ORDER.size(), JOB_DISPLAY_ORDER.size());
    unsigned y = 0;
    for(unsigned idx = 0, x = 0; idx < numElements; ++x, ++idx)
    {
        // Alternating rows with 4 and 5 items
        if(x >= (isRowWithFourElemens ? 4u : 5u))
        {
            x = 0;
            ++y;

            isRowWithFourElemens = !isRowWithFourElemens;
        }

        const Extent btSize(26, 26);
        // Content positions are relative to the tab group (inside tab control).
        // Subtract contentOffset to keep same absolute position, add TAB_HEIGHT to clear tab bar.
        const DrawPoint btPos((isRowWithFourElemens ? btSize.x + 1 : btSize.x / 2) + x * 28 - contentOffset.x,
                              topMargin - contentOffset.y + TAB_HEIGHT + y * rowHeight);

        if(idx < WARE_DISPLAY_ORDER.size())
        {
            const GoodType rawWare = WARE_DISPLAY_ORDER[idx];
            const GoodType ware = convertShieldToNation(rawWare, player.nation);
            addElement(*waresGroup, font, btPos, btSize, rttr::enum_cast(rawWare), _(WARE_NAMES[rawWare]),
                       LOADER.GetWareTex(ware), allow_outhousing);
        }

        if(idx < JOB_DISPLAY_ORDER.size())
        {
            const Job job = JOB_DISPLAY_ORDER[idx];
            addElement(*figuresGroup, font, btPos, btSize, rttr::enum_cast(job), _(JOB_NAMES[job]), LOADER.GetJobTex(job),
                       allow_outhousing);
        }
    }

    // compute the final window size (content + tab bar + buttons)
    // content starts at TAB_HEIGHT within the tab control, tab control starts at contentOffset.y
    const unsigned contentHeight = (y + 1) * rowHeight;
    const unsigned totalHeight = contentOffset.y + TAB_HEIGHT + topMargin + contentHeight
                                 + spacingBetweenLastRowAndButtonRow + additionalYSpace + buttonRowHeight
                                 + bottomMargin;
    Resize(Extent(GetSize().x, totalHeight));

    // "Help" button
    AddImageButton(12, DrawPoint(16, GetFullSize().y - 47), Extent(32, 32), TextureColor::Grey,
                   LOADER.GetImageN("io", 225), _("Help"));

    // Ersten Tab auswählen
    tabCtrl->SetSelection(0, false);
    curPage_ = warePageID;
}

void iwWares::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 12: // Hilfe
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("Here you will find a list of your entire stores of "
                                         "merchandise and all the inhabitants of your realm.")));
            break;
    }
}

void iwWares::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    // Farben ggf. aktualisieren

    if(!tabCtrl || (curPage_ != peoplePageID && curPage_ != warePageID))
        return;

    auto* group = tabCtrl->GetGroup(curPage_);
    if(group)
    {
        const unsigned count =
          (curPage_ == warePageID) ? helpers::NumEnumValues_v<GoodType> : helpers::NumEnumValues_v<Job>;

        for(unsigned i = 0; i < count; ++i)
        {
            auto* text = group->GetCtrl<ctrlText>(600 + i);
            if(text)
            {
                const unsigned amount =
                  (curPage_ == warePageID) ? inventory[static_cast<GoodType>(i)] : inventory[static_cast<Job>(i)];
                text->SetText(std::to_string(amount));
                text->SetTextColor((amount == 0) ? COLOR_RED : COLOR_YELLOW);

                if(leatheraddon::isAddonActive(player.GetGameWorld()))
                {
                    if(peoplePageID == curPage_ && isSoldier(static_cast<Job>(i)))
                    {
                        auto* tooltip = group->GetCtrl<ctrlBaseTooltip>(100 + i);
                        std::string toolTip = _(JOB_NAMES[static_cast<Job>(i)]);

                        toolTip += std::string(" (")
                                   + std::to_string(inventory[jobEnumToAmoredSoldierEnum(static_cast<Job>(i))])
                                   + std::string("/") + std::to_string(amount) + std::string(" ")
                                   + std::string(_("with armor)"));

                        tooltip->SetTooltip(toolTip);
                    }
                }
            }
        }
    }
}

/**
 *  bestimmte Inventurseite zeigen.
 *
 *  @param[in] page Die neue Seite (Tab-ID)
 */
void iwWares::SetPage(unsigned page)
{
    curPage_ = page;
    if(tabCtrl)
        tabCtrl->SetSelectionByID(page, false);
}

ctrlGroup& iwWares::AddPage(glArchivItem_Bitmap* image, const std::string& tooltip, unsigned& tabId)
{
    tabId = nextTabId;
    ctrlGroup* grp = tabCtrl->AddTab(image, tooltip, nextTabId);
    nextTabId++;
    if(!grp)
        throw std::runtime_error("Failed to add tab page");
    return *grp;
}

void iwWares::Msg_TabChange(unsigned ctrl_id, unsigned short tab_id)
{
    if(ctrl_id == 0) // tabCtrl ID
    {
        SetPage(tab_id);
    }
}
