#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <string>

#include "imgui.h"
#include "imgui_panels.h"
#include "arcdps_datastructures.h"
#include "mechanics.h"
#include "player.h"

/* proto/globals */
uint32_t cbtcount = 0;
arcdps_exports arc_exports;
char* arcvers;
void dll_init(HANDLE hModule);
void dll_exit();
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, char* skillname);
uintptr_t mod_imgui();
uintptr_t mod_options();

uint64_t start_time = 0;

std::string print_buffer = "";
static bool show_app_log;

inline int get_elapsed_time(uint64_t &current_time)
{
    return (current_time-start_time)/1000;
}

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
	switch (ulReasonForCall) {
		case DLL_PROCESS_ATTACH: dll_init(hModule); break;
		case DLL_PROCESS_DETACH: dll_exit(); break;

		case DLL_THREAD_ATTACH:  break;
		case DLL_THREAD_DETACH:  break;
	}
	return 1;
}

/* dll attach -- from winapi */
void dll_init(HANDLE hModule) {
	return;
}

/* dll detach -- from winapi */
void dll_exit() {
	return;
}

/* export -- arcdps looks for this exported function and calls the address it returns */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext) {
	arcvers = arcversionstr;
	ImGui::SetCurrentContext((ImGuiContext*)imguicontext);
	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns */
extern "C" __declspec(dllexport) void* get_release_addr() {
	arcvers = 0;
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init()
{
	/* for arcdps */
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = "Mechanics Log";
	arc_exports.out_build = "0.2";
	arc_exports.sig = 0x81004122;//from random.org
	arc_exports.wnd = mod_wnd;
	arc_exports.combat = mod_combat;
	arc_exports.imgui = mod_imgui;
	arc_exports.options = mod_options;
	return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release()
{
	return 0;
}

/* window callback -- return is assigned to umsg (return zero to not be processed by arcdps or game) */
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if 0
	/* big buffer */
	char buff[4096];
	char* p = &buff[0];

	/* common */
	p += _snprintf(p, 400, "==== wndproc %llx ====\n", hWnd);
	p += _snprintf(p, 400, "umsg %u, wparam %lld, lparam %lld\n", uMsg, wParam, lParam);

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], p - &buff[0], &written, 0);
#endif
	return uMsg;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, char* skillname)
{
	Player* current_player = nullptr;
	std::string output = "";

	/* ev is null. dst will only be valid on tracking add. skillname will also be null */
	if (!ev)
    {

	}

	/* combat event. skillname may be null. non-null skillname will remain static until module is unloaded. refer to evtc notes for complete detail */
	else
    {

		/* common */

		/* statechange */
		if (ev->is_statechange)
        {
            if(ev->is_statechange==1
                && src->self)
            {
                    start_time = ev->time;
                    reset_all_player_stats();
                    if(has_logged_mechanic)
                    {
                        has_logged_mechanic = false;
                        output += "===========\n";
                    }
            }
		}

		/* activation */
		else if (ev->is_activation)
        {

		}

		/* buff remove */
		else if (ev->is_buffremove)
        {
            if(is_player(dst))
            {
                if (ev->skillid==1122)//if it's stability
                {
                    current_player=get_player(ev->dst_instid);
                    if(current_player)
                    {
                        current_player->last_stab_time = ev->time+ms_per_tick;//cut the ending time of stab early
                    }
                }
            }
		}

		/* buff */
		else if (ev->buff)
        {
            if(is_player(dst))
            {
                if (ev->skillid==1122)//if it's stability
                {
                    current_player=get_player(ev->dst_instid);
                    if(current_player
                       && current_player->last_stab_time < (ev->time+ev->value))//if the new stab will last longer than any possible old stab
                    {
                        current_player->last_stab_time = ev->time+ev->value+ms_per_tick;//add prediction of when new stab will end
                    }
                }
            }
		}

        if(ev->dst_agent)
        {
            //if attack hits (not block/evaded/invuln/miss)
            //and it's a player, not a summon
            if(ev->result==0 || ev->result==1 || ev->result==2 || ev->result==5 || ev->result==8)
            {

                for(uint16_t index=0;index<mechanics.size();index++)
                {
                    if(mechanics[index].is_valid_hit(ev, src, dst))
                    {
                        output += std::to_string(get_elapsed_time(ev->time)/60);
                        output += ":";
                        if(get_elapsed_time(ev->time)%60 < 10)
                        {
                            output += "0";
                        }
                        output += std::to_string(get_elapsed_time(ev->time)%60);
                        output += " - ";
                        if(mechanics[index].target_is_dst)
                        {
                            output += dst->name;
                        }
                        else
                        {
                            output += src->name;
                        }
                        output += " ";
                        output += mechanics[index].name;
                        output += "\n";
                    }
                }
            }
        }

		/* common */
		cbtcount += 1;

		if(!have_added_line_break
        && ev->time > (last_mechanic_time + line_break_frequency))
        {
            have_added_line_break = true;
            output += "\n";
        }
	}

    print_buffer +=output;
	return 0;
}

static void ShowMechanicsLog(bool* p_open)
{
    static ExampleAppLog log;

    log.AddLog(print_buffer.c_str());
    print_buffer = "";

    if(show_app_log) log.Draw("MECHANICS", p_open);
}

uintptr_t mod_imgui()
{
    ShowMechanicsLog(&show_app_log);

    return 0;
}

uintptr_t mod_options()
{
    ImGui::Checkbox("MECHANICS LOG", &show_app_log);

    return 0;
}
