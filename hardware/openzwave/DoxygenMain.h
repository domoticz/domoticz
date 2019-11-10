/**
 * \mainpage OpenZWave
 * \section Introduction Introduction to OpenZWave
 * OpenZWave is an open-source, cross-platform library designed to enable
 * anyone to add support for Z-Wave home-automation devices to their 
 * applications, without requiring any in depth knowledge of the Z-Wave
 * protocol.
 * <p>
 * For more information about the OpenZWave project: see README.md in the
 * root of the OpenZWave repository.
 * \section ZWave Z-Wave Concepts
 * Z-Wave is a proprietary wireless communications protocol employing mesh
 * networking technology.  A mesh network allows low power devices to 
 * communicate over long ranges, and around radio black spots by passing
 * messages from one node to another.  It is important to note that not all
 * Z-Wave devices are active all the time, especially those that are battery
 * powered.  These nodes cannot take part in the forwarding of messages
 * across the mesh.
 * <p>
 * Each Z-Wave device is known as "Node" in the network.  A Z-Wave network
 * can contain up to 232 nodes.  If more devices are required, then
 * multiple networks need to be set up using separate Z-Wave controllers.
 * OpenZWave supports multiple controllers, but on its own does not bridge
 * the networks, allowing a device on one to directly control a device on
 * another. This functionality would have to be supplied by the application.
 * <p>
 * Z-Wave nodes can be divided into two types: Controllers and Slaves.  The
 * controllers are usually in the form of hand-held remote controls, or PC
 * interfaces.  The switches, dimmers, movement sensors etc are all slaves.
 * Controllers and Devices
 * Replication
 * Command Classes
 * Values
 * <hr>
 * \section Library The OpenZWave Library
 * \subsection Overview Overview

 * \subsection Manager The Manager
 * All Z-Wave functionality is accessed via the Manager class.  While this
 * does not make for the most efficient code structure, it does enable 
 * the library to handle potentially complex and hard-to-debug issues 
 * such as multi-threading and object lifespans behind the scenes.
 * Application development is therefore simplified and less prone to bugs.
 * <p>

 * \subsection Notifications Notifications
 * Communication between the PC and devices on the the Z-Wave network occurs
 * asynchronously.  Some devices, notably movement sensors, sleep most of the
 * time to save battery power, and can only receive commands when awake.
 * Therefore a command to change a value in a device may not occur immediately.
 * It may take several seconds or minutes, or even never arrive at all.
 * For this reason, many OpenZWave methods, even those that request
 * information, do not return that information directly.  A request will be 
 * sent to the network, and the response sent to the application some time
 * later via a system of notification callbacks.  The notification handler
 * will be at the core of any application using OpenZWave.  It is here that
 * all information regarding device configurations and states will be reported.
 * <p>
 * A Z-Wave network is a dynamic entity.  Controllers and devices can be
 * added or removed at any time.  Once a network has been set up, this
 * probably won't happen very often, but OpenZWave, and any application
 * built upon it, must cope all the same.  The notification callback system
 * is used to inform the application of any changes to the structure of the
 * network.

 * \subsection Values Working with Values
 *

 * \subsection Guidelines Application Development Guidelines
 * The main considerations when designing an application based upon 
 * OpenZWave are:
 *
 * - Communication with Z-Wave devices is asynchronous and not guaranteed 
 * to be reliable.  Do not rely on receiving a response to any request.
 *
 * - A Z-Wave network may change at any time.  The application's notification
 * callback handler must deal with all notifications, and any representation
 * of the state of the Z-Wave network held by the application must be
 * modified to match.  User interfaces should be built dynamically from the
 * information reported in the notification callbacks, and must be able to cope
 * with the addition and removal of devices.
 *
 * Some users will have more than one Z-Wave controller, to allow for remote
 * locations or to work around the 232 device limit.  OpenZWave is designed
 * for use with multiple controllers, and all applications should be written
 * to support this.
 *
 * - There is no need to save the state of the network and restore it on
 * the next run. OpenZWave does this automatically, and is designed to cope
 * with any changes that occur to the network while the application is not
 * running.
 *
 
 * <hr>
 * \section Licensing Licensing
 * See README.md in the root of the OpenZWave project.
 
 * <hr>
 * \section Support Support
 * See README.md in the root of the OpenZWave project.
 * <p>
 */

