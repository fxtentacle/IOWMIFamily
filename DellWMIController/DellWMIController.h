/*
 *  DellWMIcontroller.h
 *  IOWMIFamily
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DellWMIController_h
#define _DellWMIController_h

#include <IOKit/IOService.h>

#include "IOWMIController.h"
#include  "WMIHIKeyboardDevice.h"

class DellWMIController : public IOWMIController
{
	
	OSDeclareDefaultStructors(DellWMIController)
	
public:
	virtual bool       init(OSDictionary *dictionary = 0);
	virtual bool       start(IOService *provider);
	virtual void       stop(IOService *provider);
	virtual void       free(void);	
	virtual IOService * probe(IOService *provider, SInt32 *score );
	
protected:
	virtual void enableEvent();
	virtual void disableEvent();
	virtual void handleMessage(int code);
	
	static const wmiKeyMap keyMap[];
	
};

#endif
