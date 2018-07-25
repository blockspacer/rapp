//--------------------------------------------------------------------------//
/// Copyright (c) 2018 by Milos Tosic. All Rights Reserved.                ///
/// License: http://www.opensource.org/licenses/BSD-2-Clause               ///
//--------------------------------------------------------------------------//

#include <rapp_pch.h>
#include <inttypes.h>

#define RTM_LIBHANDLER_DEFINE
#include <rbase/inc/libhandler.h>

#include <rapp/inc/rapp.h>
#include <rapp/src/entry_p.h>
#include <rapp/src/cmd.h>
#include <rapp/src/input.h>
#include <rapp/src/job.h>

namespace rapp
{
	int rapp_main(int _argc, const char* const*);

	struct CmdContext;

#if RAPP_WITH_BGFX
	extern uint32_t g_debug;
	extern uint32_t g_reset;
#else
	extern uint32_t g_debug;
	extern uint32_t g_reset;
#endif

	static const char* s_keyName[] =
	{
		"None",
		"Esc",
		"Return",
		"Tab",
		"Space",
		"Backspace",
		"Up",
		"Down",
		"Left",
		"Right",
		"Insert",
		"Delete",
		"Home",
		"End",
		"PageUp",
		"PageDown",
		"Print",
		"Plus",
		"Minus",
		"LeftBracket",
		"RightBracket",
		"Semicolon",
		"Quote",
		"Comma",
		"Period",
		"Slash",
		"Backslash",
		"Tilde",
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
		"NumPad0", "NumPad1", "NumPad2", "NumPad3", "NumPad4",
		"NumPad5", "NumPad6", "NumPad7", "NumPad8", "NumPad9",
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
		"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
		"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
	};
	RTM_STATIC_ASSERT(KeyboardState::Key::Count == RTM_NUM_ELEMENTS(s_keyName) );

	const char* getName(KeyboardState::Key _key)
	{
		RTM_ASSERT(_key < KeyboardState::Key::Count, "Invalid key %d.", _key);
		return s_keyName[_key];
	}

#if RAPP_WITH_BGFX
	static const InputBinding s_bindingsGraphics[] =
	{
		{ NULL, "graphics fullscreen",               1, { KeyboardState::Key::KeyF,   KeyboardState::Modifier::LCtrl  }},
		{ NULL, "graphics fullscreen",               1, { KeyboardState::Key::KeyF,   KeyboardState::Modifier::RCtrl  }},
		{ NULL, "graphics fullscreen",               1, { KeyboardState::Key::F11,    KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics stats",                    1, { KeyboardState::Key::F1,     KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics stats 0\ngraphics text 0", 1, { KeyboardState::Key::F1,     KeyboardState::Modifier::LShift }},
		{ NULL, "graphics wireframe",                1, { KeyboardState::Key::F3,     KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics hmd",                      1, { KeyboardState::Key::F4,     KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics hmdrecenter",              1, { KeyboardState::Key::F4,     KeyboardState::Modifier::LShift }},
		{ NULL, "graphics hmddbg",                   1, { KeyboardState::Key::F4,     KeyboardState::Modifier::LCtrl  }},
		{ NULL, "graphics vsync",                    1, { KeyboardState::Key::F7,     KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics msaa16",                   1, { KeyboardState::Key::F8,     KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics flush",                    1, { KeyboardState::Key::F9,     KeyboardState::Modifier::NoMods }},
		{ NULL, "graphics screenshot",               1, { KeyboardState::Key::Print,  KeyboardState::Modifier::NoMods }},

		RAPP_INPUT_BINDING_END
	};
#endif

	int main(int _argc, const char* const* _argv)
	{
		cmdInit();
		cmdAdd("mouselock", cmdMouseLock, 0, "locks mouse to window");
#if RAPP_WITH_BGFX
		cmdAdd("graphics",  cmdGraphics,  0, "Graphics related commands, type 'graphics help' for list of options");
		rapp::inputAddBindings("graphics", s_bindingsGraphics);
#endif
		rapp::jobInit();

		WindowHandle defaultWindow = { 0 };

		const char* executable = rtm::pathGetFileName(_argv[0]);
		windowSetTitle(defaultWindow, executable ? executable : "rapp");

		int32_t result = rapp::rapp_main(_argc, _argv);

		rapp::jobShutdown();

#if RAPP_WITH_BGFX
		rapp::inputRemoveBindings("graphics");
#endif
		cmdShutdown();
	
		return result;
	}

	bool processEvents(App* _app)
	{
		uint32_t debug = g_debug;
		uint32_t reset = g_reset;

		WindowHandle handle = { UINT16_MAX };

		const Event* ev;
		do
		{
#if RAPP_WITH_BGFX
			bool keysBindings =
#endif // RAPP_WITH_BGFX
			inputProcess(_app);

			inputResetMouseMovement();
			inputResetGamepadAxisMovement();

			struct SE { const Event* m_ev; SE() : m_ev(poll() ) {} ~SE() { if (NULL != m_ev) { release(m_ev); } } } scopeEvent;
			ev = scopeEvent.m_ev;

			if (NULL != ev)
			{
				switch (ev->m_type)
				{
				case Event::Axis:
					{
						const AxisEvent* axis = static_cast<const AxisEvent*>(ev);
						inputSetGamepadAxis(axis->m_gamepad, axis->m_axis, axis->m_value);
					}
					break;

				case Event::Char:
					{
						const CharEvent* chev = static_cast<const CharEvent*>(ev);
						inputChar(chev->m_len, chev->m_char);
#if RAPP_WITH_BGFX
						if (ImGui::GetIO().WantCaptureKeyboard && (keysBindings == false))
						ImGui::GetIO().AddInputCharactersUTF8((const char*)chev->m_char);
#endif // RAPP_WITH_BGFX

					}
					break;

				case Event::Exit:
					return true;

				case Event::Gamepad:
					{
						const GamepadEvent* gev = static_cast<const GamepadEvent*>(ev);
						inputSetGamepadConnected(gev->m_gamepad, gev->m_connected);
						RAPP_DBG("gamepad %d, %d", gev->m_gamepad.idx, gev->m_connected);
					}
					break;

				case Event::GamepadButton:
					{
						const GamepadButtonEvent* gev = static_cast<const GamepadButtonEvent*>(ev);
						inputSetGamepadButtonState(gev->m_gamepad, gev->m_button, gev->m_pressed);
					}
					break;

				case Event::Mouse:
					{
						const MouseEvent* mouse = static_cast<const MouseEvent*>(ev);
						handle = mouse->m_handle;

						if (mouse->m_move)
						{
							inputSetMousePos(mouse->m_mx, mouse->m_my, mouse->m_mz);
						}
						else
						{
							inputSetMouseButtonState(mouse->m_button, mouse->m_down);
						}
					}
					break;

				case Event::Key:
					{
						const KeyEvent* key = static_cast<const KeyEvent*>(ev);
						handle = key->m_handle;
						inputSetKeyState(key->m_key, key->m_modifiers, key->m_down);
#if RAPP_WITH_BGFX
						ImGui::GetIO().KeysDown[key->m_key] = key->m_down;
#endif // RAPP_WITH_BGFX
					}
					break;

				case Event::Size:
					{
						const SizeEvent* size = static_cast<const SizeEvent*>(ev);
						handle  = size->m_handle;
						_app->m_width  = size->m_width;
						_app->m_height = size->m_height;
						reset  = !g_reset; // force reset
					}
					break;

				case Event::Window:
					break;

				default:
					break;
				}
			}

		} while (NULL != ev);

		if ((handle.idx == 0) && (reset != g_reset))
		{
			reset = g_reset;
#if RAPP_WITH_BGFX
			bgfx::reset(_app->m_width, _app->m_height, reset);
#endif
			inputSetMouseResolution((uint16_t)_app->m_width, (uint16_t)_app->m_height);
		}

		g_debug = debug;

		return _app->m_exitCode != -1;
	}

} // namespace rapp
