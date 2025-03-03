/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <settings/Registered.hpp>
#include <MiscTemporary.hpp>
#include "HookedMethods.hpp"
#include "CatBot.hpp"
#include "drawmgr.hpp"

static settings::Boolean debug_log_panel_names{ "debug.log-panels", "false" };

static settings::Int waittime{ "debug.join-wait-time", "2500" };
int spamdur = 0;
Timer joinspam{};
CatCommand join_spam("join_spam", "Spam joins server for X seconds",
                     [](const CCommand &args)
                     {
                         if (args.ArgC() < 2)
                             return;
                         int id = atoi(args.Arg(1));
                         joinspam.update();
                         spamdur = id;
                     });
CatCommand join("mm_join", "Join mm Match",
                []()
                {
                    auto gc = re::CTFGCClientSystem::GTFGCClientSystem();
                    if (gc)
                        gc->JoinMMMatch();
                });

bool replaced = false;
namespace hooked_methods
{
DEFINE_HOOKED_METHOD(PaintTraverse, void, vgui::IPanel *this_, vgui::VPANEL panel, bool force, bool allow_force)
{
    if (!isHackActive())
        return original::PaintTraverse(this_, panel, force, allow_force);

    static bool textures_loaded       = false;
    static vgui::VPANEL panel_scope   = 0;
    static vgui::VPANEL motd_panel    = 0;
    static vgui::VPANEL motd_panel_sd = 0;
    static vgui::VPANEL health_panel  = 0;
    static bool call_default          = true;
    static ConVar *software_cursor    = g_ICvar->FindVar("cl_software_cursor");

#if ENABLE_VISUALS
    if (!textures_loaded)
        textures_loaded = true;
#endif
    static bool switcherido = false;
    static int scndwait     = 0;
    if (scndwait > int(waittime))
    {
        scndwait = 0;
        if (switcherido && spamdur && !joinspam.check(spamdur * 1000))
        {
            auto gc = re::CTFGCClientSystem::GTFGCClientSystem();
            if (gc)
                gc->JoinMMMatch();
        }
        else if (!joinspam.check(spamdur * 1000) && spamdur)
        {
            INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
            if (ch)
                ch->Shutdown("");
        }
    }
    scndwait++;
    switcherido = !switcherido;
    call_default = true;
    if (isHackActive() && (health_panel || panel_scope || motd_panel || motd_panel_sd) && (*hacks::catbot::catbotmode && *hacks::catbot::anti_motd && (panel == motd_panel || panel == motd_panel_sd)))
        call_default = false;

    if (call_default)
        original::PaintTraverse(this_, panel, force, allow_force);

    if (debug_log_panel_names)
        logging::Info("Panel name: %s %lu", g_IPanel->GetName(panel), panel);
    if (!panel_scope)
        if (!strcmp(g_IPanel->GetName(panel), "HudScope"))
            panel_scope = panel;
    if (!motd_panel)
        if (!strcmp(g_IPanel->GetName(panel), "info"))
            motd_panel = panel;
    if (!motd_panel_sd)
        if (!strcmp(g_IPanel->GetName(panel), "ok"))
            motd_panel_sd = panel;
    if (!health_panel)
        if (!strcmp(g_IPanel->GetName(panel), "HudPlayerHealth"))
            health_panel = panel;
#if ENABLE_ENGINE_DRAWING
    if (!FocusOverlayPanel)
    {
        const char *szName = g_IPanel->GetName(panel);
        if (szName[0] == 'F' && szName[5] == 'O' && szName[12] == 'P')
        {
            FocusOverlayPanel = panel;
        }
    }
    if (FocusOverlayPanel == panel)
    {
        g_IPanel->SetTopmostPopup(FocusOverlayPanel, true);
        render_cheat_visuals();
    }
#endif

    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid = true;
    }

    draw::UpdateWTS();
}
} // namespace hooked_methods