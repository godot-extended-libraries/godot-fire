#ifndef DBUS_CPP
#define DBUS_CPP

#include "core/reference.h"
#include "scene/main/node.h"

#include "echo_service.h"

#include "simppl/dispatcher.h"
#include "simppl/skeleton.h"
#include "simppl/string.h"
#include "simppl/stub.h"

#include <iostream>
#include <chrono>

class MyEchoClient : public simppl::dbus::Stub<simppl::example::EchoService> {

public:
	MyEchoClient(simppl::dbus::Dispatcher &disp) :
			simppl::dbus::Stub<simppl::example::EchoService>(disp, "myEcho") {
		connected >> [this](simppl::dbus::ConnectionState st) { handle_connected(st); };
	}

private:
	void handle_connected(simppl::dbus::ConnectionState connect_st) {
		if (connect_st == simppl::dbus::ConnectionState::Connected) {
			echo.async("Hello World!") >> [this](const simppl::dbus::CallState st, const std::string &echo_string) {
				if (st) {
					std::cout << "Server says '" << echo_string << "'" << std::endl;
				} else
					std::cout << "Got error: " << st.what() << std::endl;

				disp().stop();
			};
		}
	}
};

class MyEcho : public simppl::dbus::Skeleton<simppl::example::EchoService> {
public:
	MyEcho(simppl::dbus::Dispatcher &disp) :
			simppl::dbus::Skeleton<simppl::example::EchoService>(disp, "myEcho") {
		echo >> [this](const std::string &echo_string) {
			std::cout << "Client says '" << echo_string << "'" << std::endl;
			respond_with(echo(echo_string));
		};
	}
};

class Dbus : public Node {
	GDCLASS(Dbus, Node);
    friend class simppl::dbus::Dispatcher;

protected:
	static void _bind_methods();
	simppl::dbus::Dispatcher disp;
	MyEchoClient client;

public:
	void _notification(int p_what) {
		if (p_what == NOTIFICATION_READY) {
			disp.init();
		} else if (p_what == NOTIFICATION_INTERNAL_PHYSICS_PROCESS) {          
            int64_t milli = get_physics_process_delta_time() / 1000.0f; 
            std::chrono::milliseconds sec(milli);   
			disp.step(sec);
		}
	}
	Dbus() : disp("bus:session"), client(disp) {
    }
};

#endif // SUMMATOR_H
