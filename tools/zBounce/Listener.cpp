/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * zBounce - A compressing TCP port bouncer.
 *
 * This tool is designed to set up a number of local listening ports, and
 * then forward any data recived on those ports to another host/port combo.
 * (Optionally compressing the data between them).
 * Each listening port can bounce to a different host/port defined in the
 * config file.
 *
 * -- Greg Sikorski. (gte@atomicrevs.demon.co.uk).
 *
 * See Changelog for detailed project history.
 *
 * $Id$ 
 *
 */

#include <new>
#include <list>
#include <string>
#include <fstream>
#include <iostream>

#include <cerrno>

#include "Bounce.h"
#include "Listener.h"
#include "Socket.h"
#include "main.h"

using std::string;
using std::cerr;
using std::endl;
using std::ifstream;
using std::cout;

Socket *Listener::handleAccept() {
  /*
   *  handleAccept.
   *  Inputs: Nothing.
   *  Outputs: A Socket Object.
   *  Process: 1. Accept's an incomming connection,
   *              and returns a new socket object. 
   */

  socklen_t sin_size = static_cast<socklen_t>(sizeof(struct sockaddr_in));

  Socket *newSocket = new (std::nothrow) Socket();

  if (NULL == newSocket) {
    cerr << "Listener::handleAccept> Memory allocation failure\n";
    ::exit(0);
  } // if

  int new_fd = ::accept(myFD, reinterpret_cast<struct sockaddr *>(newSocket->getAddress()), &sin_size);

  newSocket->setFD(new_fd);

  // TOOD: Where did this number come from? -dan
  unsigned int opt = 61440;

  ::setsockopt(new_fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
  ::setsockopt(new_fd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));

  return newSocket;
} // Listener::handleAccept
 
void Listener::beginListening() {
  /*
   *  beginListening.
   *  Inputs: Nothing.
   *  Outputs: Nothing.
   *  Process: 1. Binds the local ports for all the
   *              Listener objects.
   *
   */

  myFD = ::socket(AF_INET, SOCK_STREAM, 0);
  if (myFD < 0) {
	cerr << "Listener::beginListening> Unable to allocate socket: "
             << ::strerror( errno ) << endl;

    return;
  } // if

  struct sockaddr_in my_addr;
  int optval = 1;

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(getLocalPort());
  my_addr.sin_addr.s_addr = ::inet_addr(getVHost().c_str());
  ::memset( &(my_addr.sin_zero), 0, 8);

  ::setsockopt(myFD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  int bindRes = ::bind(myFD, reinterpret_cast< struct sockaddr *>(&my_addr), sizeof(struct sockaddr));

  if (bindRes < 0) {
    /*
     *  If we can't bind a listening port, we might aswell drop out.
     */
    aBounce->logEntry("Listener::beginListening> Unable to bind to %s:%i!", getVHost().c_str(), getLocalPort());

    ::exit(0);
  } // if

  ::listen(myFD, 10);
} // Listener::beginListening
