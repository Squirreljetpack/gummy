/**
* gummy
* Copyright (C) 2023  Francesco Fusco
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DBUS_HPP
#define DBUS_HPP

#include <sdbus-c++/IProxy.h>

inline std::unique_ptr<sdbus::IProxy> dbus_register_signal_handler(
    std::string service,
    std::string obj_path,
    std::string interface,
    std::string signal_name,
    std::function<void(sdbus::Signal &signal)> handler)
{
	auto proxy = sdbus::createProxy(service, obj_path);
	proxy->registerSignalHandler(interface, signal_name, handler);
	proxy->finishRegistration();
	return proxy;
}


#endif // DBUS_HPP
