/*
 *  IOWMIController.cpp
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

#include <IOKit/hidsystem/ev_keymap.h>

#include "IOWMIController.h"
#include "debug.h"

	
/*
 * GUID parsing functions
 */
#ifdef DEBUG
/**
 * wmi_parse_hexbyte - Convert a ASCII hex number to a byte
 * @src:  Pointer to at least 2 characters to convert.
 *
 * Convert a two character ASCII hex string to a number.
 *
 * Return:  0-255  Success, the byte was parsed correctly
 *          -1     Error, an invalid character was supplied
 */
int IOWMIController::wmi_parse_hexbyte(const UInt8 *src)
{
	unsigned int x; /* For correct wrapping */
	int h;
	
	/* high part */
	x = src[0];
	if (x - '0' <= '9' - '0') {
		h = x - '0';
	} else if (x - 'a' <= 'f' - 'a') {
		h = x - 'a' + 10;
	} else if (x - 'A' <= 'F' - 'A') {
		h = x - 'A' + 10;
	} else {
		return -1;
	}
	h <<= 4;
	
	/* low part */
	x = src[1];
	if (x - '0' <= '9' - '0')
		return h | (x - '0');
	if (x - 'a' <= 'f' - 'a')
		return h | (x - 'a' + 10);
	if (x - 'A' <= 'F' - 'A')
		return h | (x - 'A' + 10);
	return -1;
}


/**
 * wmi_swap_bytes - Rearrange GUID bytes to match GUID binary
 * @src:   Memory block holding binary GUID (16 bytes)
 * @dest:  Memory block to hold byte swapped binary GUID (16 bytes)
 *
 * Byte swap a binary GUID to match it's real GUID value
 */
void IOWMIController::wmi_swap_bytes(UInt8 *src, UInt8 *dest)
{
	int i;
	
	for (i = 0; i <= 3; i++)
		memcpy(dest + i, src + (3 - i), 1);
	
	for (i = 0; i <= 1; i++)
		memcpy(dest + 4 + i, src + (5 - i), 1);
	
	for (i = 0; i <= 1; i++)
		memcpy(dest + 6 + i, src + (7 - i), 1);
	
	memcpy(dest + 8, src + 8, 8);
}


/**
 * wmi_parse_guid - Convert GUID from ASCII to binary
 * @src:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba
 * @dest:  Memory block to hold binary GUID (16 bytes)
 *
 * N.B. The GUID need not be NULL terminated.
 *
 * Return:  'true'   @dest contains binary GUID
 *          'false'  @dest contents are undefined
 */
bool IOWMIController::wmi_parse_guid(const UInt8 *src, UInt8 *dest)
{
	static const int size[] = { 4, 2, 2, 2, 6 };
	int i, j, v;
	
	if (src[8]  != '-' || src[13] != '-' ||
		src[18] != '-' || src[23] != '-')
		return false;
	
	for (j = 0; j < 5; j++, src++) {
		for (i = 0; i < size[j]; i++, src += 2, *dest++ = v) {
			v = wmi_parse_hexbyte(src);
			if (v < 0)
				return false;
		}
	}
	
	return true;
}


/**
 * wmi_dump_wdg - dumps tables to dmesg
 * @src: guid_block *
 */
void IOWMIController::wmi_dump_wdg(struct guid_block *g)
{
	char guid_string[37];
	
	wmi_data2Str(g->guid, guid_string);
	IOLog("%s:\n", guid_string);
	if (g->flags & ACPI_WMI_EVENT)
		IOLog("\tnotify_value: %02X\n", g->notify_id);
	else
		IOLog("\tobject_id: %c%c\n",g->object_id[0], g->object_id[1]);
	IOLog("\tinstance_count: %d\n", g->instance_count);
	IOLog("\tflags: %#x", g->flags);
	if (g->flags) {
		IOLog(" ");
		if (g->flags & ACPI_WMI_EXPENSIVE)
			IOLog("ACPI_WMI_EXPENSIVE ");
		if (g->flags & ACPI_WMI_METHOD)
			IOLog("ACPI_WMI_METHOD ");
		if (g->flags & ACPI_WMI_STRING)
			IOLog("ACPI_WMI_STRING ");
		if (g->flags & ACPI_WMI_EVENT)
			IOLog("ACPI_WMI_EVENT ");
	}
	IOLog("\n");
}
#endif


/**
 * wmi_data2Str - converts binary guid to ascii guid
 * 
 */
int IOWMIController::wmi_data2Str(const char *in, char *out)
{
	int i;
	
	for (i = 3; i >= 0; i--)
		out += snprintf(out, 3, "%02X", in[i] & 0xFF);
	
	out += snprintf(out, 2, "-");
	out += snprintf(out, 3, "%02X", in[5] & 0xFF);
	out += snprintf(out, 3, "%02X", in[4] & 0xFF);
	out += snprintf(out, 2, "-");
	out += snprintf(out, 3, "%02X", in[7] & 0xFF);
	out += snprintf(out, 3, "%02X", in[6] & 0xFF);
	out += snprintf(out, 2, "-");
	out += snprintf(out, 3, "%02X", in[8] & 0xFF);
	out += snprintf(out, 3, "%02X", in[9] & 0xFF);
	out += snprintf(out, 2, "-");
	
	for (i = 10; i <= 15; i++)
		out += snprintf(out, 3, "%02X", in[i] & 0xFF);
	
	*out = '\0';
	return 0;
}


/**
 * flagsToStr - converts binary flag to ascii flag
 * 
 */
OSString * IOWMIController::flagsToStr(UInt8 flags)
{
	char str[80];
	char *pos = str;
	if (flags != 0)
	{
		if (flags & ACPI_WMI_EXPENSIVE)
		{
			strncpy(pos, "ACPI_WMI_EXPENSIVE ", 20);
			pos += strlen(pos);
		}
		if (flags & ACPI_WMI_METHOD)
		{
			strncpy(pos, "ACPI_WMI_METHOD ", 20);
			pos += strlen(pos);
		}
		if (flags & ACPI_WMI_STRING)
		{
			strncpy(pos, "ACPI_WMI_STRING ", 20);
			pos += strlen(pos);
		}
		if (flags & ACPI_WMI_EVENT)
		{
			strncpy(pos, "ACPI_WMI_EVENT ", 20);
			pos += strlen(pos);
		}
		//suppress the last trailing space
		str[strlen(str)] = 0;
	}
	else
	{
		str[0] = 0;
	}
	return (OSString::withCString(str));
}


/**
 * wmi_wdg2reg - adds the WDG structure to a dictionary
 * 
 */
void IOWMIController::wmi_wdg2reg(struct guid_block *g, OSArray *array, OSArray *dataArray)
{
	char guid_string[37];
	char object_id_string[3];
	OSDictionary *dict = OSDictionary::withCapacity(6);
	
	wmi_data2Str(g->guid, guid_string);
	
	dict->setObject("UUID", OSString::withCString(guid_string));
	if (g->flags & ACPI_WMI_EVENT)
		dict->setObject("notify_value", OSNumber::withNumber(g->notify_id, 8));
	else
	{
		snprintf(object_id_string, 3, "%c%c", g->object_id[0], g->object_id[1]);
		dict->setObject("object_id", OSString::withCString(object_id_string));	
	}
	dict->setObject("instance_count", OSNumber::withNumber(g->instance_count, 8));
	dict->setObject("flags", OSNumber::withNumber(g->flags, 8));
#if DEBUG 
	dict->setObject("flags Str", flagsToStr(g->flags));
#endif	
	if (g->flags == 0)
		dataArray->setObject(readDataBlock(object_id_string));

	
	array->setObject(dict);
}


OSDictionary * IOWMIController::readDataBlock(char *str)
{
	OSObject	*wqxx;
	OSData		*data = NULL;
	OSDictionary *dict;
	char name[5];
	
	snprintf(name, 5, "WQ%s", str);
	dict = OSDictionary::withCapacity(1);
	
	do
	{
		if (WMIDevice->evaluateObject(name, &wqxx) != kIOReturnSuccess)
		{
			IOLog("%s: No object of method %s\n", this->getName(), name);
			continue;
		}
		
		data = OSDynamicCast(OSData , wqxx);
		if (data == NULL){
			IOLog("%s: Cast error %s\n", this->getName(), name);
			continue;
		}
		dict->setObject(name, data);
	}
	while (false);
	return dict;
}


/*
 * Parse the _WDG method for the GUID data blocks
 */
int IOWMIController::parse_wdg(OSDictionary *dict)
{
	UInt32 i, total;
	OSObject	*wdg;
	OSData		*data;
	OSArray		*array, *dataArray;

	do
	{
		if (WMIDevice->evaluateObject("_WDG", &wdg) != kIOReturnSuccess)
		{
			IOLog("%s: No object of method _WDG\n", this->getName());
			continue;
		}
		
		data = OSDynamicCast(OSData , wdg);
		if (data == NULL){
			IOLog("%s: Cast error _WDG\n", this->getName());
			continue;
		}
		
		total = data->getLength() / sizeof(struct guid_block);
		array = OSArray::withCapacity(total);
		dataArray = OSArray::withCapacity(1);
		
		for (i = 0; i < total; i++) {
			wmi_wdg2reg((struct guid_block *) data->getBytesNoCopy(i * sizeof(struct guid_block), sizeof(struct guid_block)), array, dataArray);			
		}
		setProperty("WDG", array);
		setProperty("DataBlocks", dataArray);
		data->release();
	}
	while (false);
	
	return 0;
}


#pragma mark -
#pragma mark IOService overloading
#pragma mark -


#define super IOService

OSDefineMetaClassAndStructors(IOWMIController, IOService)


bool IOWMIController::init(OSDictionary *dict)
{
	bool result = super::init(dict);
	properties = dict;
	
	return result;
}

void IOWMIController::free(void)
{
	DbgLog("%s: Free\n", this->getName());
	super::free();
}

IOService * IOWMIController::probe(IOService *provider, SInt32 *score)
{
	IOService *result = super::probe(provider, score);
	DbgLog("%s: Probe()\n", this->getName());
	return result;
}

void IOWMIController::stop(IOService *provider)
{	
	DbgLog("%s: Stop\n", this->getName());
	
	disableEvent();
	PMstop();
	
	super::stop(provider);
	return;
}

static IOPMPowerState powerStateArray[ kPowerStateCount ] =
{
	{ 1,0,0,0,0,0,0,0,0,0,0,0 },
	{ 1,IOPMDeviceUsable,IOPMPowerOn,IOPMPowerOn,0,0,0,0,0,0,0,0 }
};


bool IOWMIController::start(IOService *provider)
{	
	if(!provider || !super::start( provider )) 
	{
		DbgLog("%s: Error loading kext\n", this->getName());
		return false;
	}
	
	WMIDevice = (IOACPIPlatformDevice *) provider;		// ACPI device
	
	IOLog("%s: WMI Device: %s\n", this->getName(), WMIDevice->getName());
	
	_keyboardDevice = NULL;
	
	parse_wdg(properties);
	
	enableEvent();
	
	PMinit();
    registerPowerDriver(this, powerStateArray, 2);
	provider->joinPMtree(this);
	
	this->registerService(0);
	
	return true;
}

/*
 * Computer power state hook
 * Nothing to do for the moment
 * 
 */
IOReturn IOWMIController::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker)
{
	if (kPowerStateOff == powerStateOrdinal)
	{
		DbgLog("%s: setPowerState(kPowerStateOff)\n", this->getName());
		
	}
	else if (kPowerStateOn == powerStateOrdinal)
	{
		DbgLog("%s: setPowerState(kPowerStateOn)\n", this->getName());
		
	}
	
	return IOPMAckImplied;
}



#pragma mark -
#pragma mark IOWMIController Methods
#pragma mark -


IOReturn IOWMIController::message( UInt32 type, IOService * provider, void * argument)
{
	if (type == kIOACPIMessageDeviceNotification)
	{
		bool isNewDell = false;
        UInt32 event = *((UInt32 *) argument);
		
		OSNumber * number = OSNumber::withNumber(event,32);
        
        if (NULL != number)
        {
            //avoid parsing Dell's default AMW0 notification, set a flag to threat separately
            if (0xD0 == number->unsigned32BitValue())
            {
                isNewDell = true;
                DbgLog("%s: Detected Dell WMI notification, will parse INF2 from INFO buffer\n", this->getName());
            }
            else 
                handleMessage(number->unsigned32BitValue());
        }
        
        OSObject * wed;
		WMIDevice->evaluateObject("_WED", &wed, (OSObject**)&number,1);
		number->release();
        
        // implement check for NULL wed by lvs1974
        number = NULL;
        
		if (NULL != wed)
            number = OSDynamicCast(OSNumber, wed);
        
		if (NULL != wed && NULL == number)
        {
            //try a package
            OSArray * array = OSDynamicCast(OSArray, wed);
            if (NULL == array)
            {
                //try a buffer
                OSData * data = OSDynamicCast(OSData, wed);
                if ( (NULL == data) || (data->getLength() == 0))
                {
                    IOLog("%s: Failed to cast. _WED returned objet %s\n", this->getName(), wed->getMetaClass()->getClassName());
                    return kIOReturnError;
                }
                const char * bytes = (const char *) data->getBytesNoCopy();

                if (isNewDell) // Dell uses Buffer INFO, parse INF2 to obtain the code for message handling
                    number = OSNumber::withNumber(bytes[4],32);
                else
                    number = OSNumber::withNumber(bytes[0],32);
            }
            else
            {
                number = OSDynamicCast(OSNumber, array->getObject(0));
                if (NULL == number)
                {
                    IOLog("%s: Failed to cast. _WED returned first array element %s\n", this->getName(), array->getObject(0)->getMetaClass()->getClassName());
                    return kIOReturnError;
                }
            }
        }
		
		if (NULL != number)
        {
            handleMessage(number->unsigned32BitValue());
            number->release();
        }
	}
	else
	{	// Someone unexpected is sending us messages!
		IOLog("%s: Unexpected message, Type %x Provider %s \n", this->getName(), uint(type), provider->getName());		
	}
	
	return kIOReturnSuccess;
}


void IOWMIController::handleMessage(int code)
{
    if (_keyboardDevice)
        _keyboardDevice->keyPressed(code);
}


/*
 * 
 * Nothing to do in the base class
 */
void IOWMIController::rfkillEvent()
{

}


/*
 *
 */
void IOWMIController::getDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status)
{
	DbgLog("%s: getDeviceStatus()\n", this->getName());
	
	char method[5];
	OSObject * params[3];
	OSString *str;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return;
	
	str = OSDynamicCast(OSString, dict->getObject("object_id"));
	if (NULL == str)
		return;
	
	snprintf(method, 5, "WM%s", str->getCStringNoCopy());
		
	params[0] = OSNumber::withNumber(0x00D,32);
	params[1] = OSNumber::withNumber(methodId,32);
	params[2] = OSNumber::withNumber(deviceId,32);
	
	WMIDevice->evaluateInteger(method, status, params, 3);
	
	params[0]->release();
	params[1]->release();
	params[2]->release();
	
	return;
}


/*
 *
 */
void IOWMIController::setDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status)
{
	DbgLog("%s: setDeviceStatus()\n", this->getName());
	
	char method[5];
	char buffer[8];
	OSObject * params[3];
	OSString *str;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return;
	
	str = OSDynamicCast(OSString, dict->getObject("object_id"));
	if (NULL == str)
		return;
	
	snprintf(method, 5, "WM%s", str->getCStringNoCopy());
	
	memcpy(buffer, &deviceId, 4);
	memcpy(buffer+4, status, 4);
	
	params[0] = OSNumber::withNumber(0x00D,32);
	params[1] = OSNumber::withNumber(methodId,32);
	params[2] = OSData::withBytes(buffer, 8);

	*status = ~0;
	WMIDevice->evaluateInteger(method, status, params, 3);
	
	DbgLog("%s: setDeviceStatus Res = %x\n", this->getName(), (unsigned int)*status);
	
	params[0]->release();
	params[1]->release();
	params[2]->release();

	return;
}

void IOWMIController::setDevice(const char * guid, UInt32 methodId, UInt32 *status)
{
	DbgLog("%s: setDevice(%d)\n", this->getName(), (int)*status);
	
	char method[5];
	char buffer[4];
	OSObject * params[3];
	OSString *str;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return;
	
	str = OSDynamicCast(OSString, dict->getObject("object_id"));
	if (NULL == str)
		return;
	
	snprintf(method, 5, "WM%s", str->getCStringNoCopy());
	
	memcpy(buffer, status, 4);
	
	params[0] = OSNumber::withNumber(0x00D,32);
	params[1] = OSNumber::withNumber(methodId,32);
	params[2] = OSData::withBytes(buffer, 8);
	
	*status = ~0;
	WMIDevice->evaluateInteger(method, status, params, 3);
	
	DbgLog("%s: setDevice Res = %x\n", this->getName(), (unsigned int)*status);
	
	params[0]->release();
	params[1]->release();
	params[2]->release();
	
	return;
}


OSDictionary* IOWMIController::getDictByUUID(const char * guid)
{
	UInt32 i;
	OSDictionary	*dict = NULL;
	OSString		*uuid;
	OSArray *array = OSDynamicCast(OSArray, properties->getObject("WDG"));
	if (NULL == array)
		return NULL;
	
	for (i=0; i<array->getCount(); i++) {
		dict = OSDynamicCast(OSDictionary, array->getObject(i));
		uuid = OSDynamicCast(OSString, dict->getObject("UUID"));
		if (uuid->isEqualTo(guid))
			break;
	}
	return dict;
}


IOReturn IOWMIController::setEvent(const char * guid, bool state)
{
	DbgLog("%s: setEvent()\n", this->getName());
	
	char method[5];
	OSNumber *num;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return kIOReturnError;
	
	num = OSDynamicCast(OSNumber, dict->getObject("notify_value"));
	if (NULL == num)
		return kIOReturnError;
	
	snprintf(method, 5, "WE%02X", num->unsigned8BitValue());

	OSObject * res;
	OSNumber * number = OSNumber::withNumber(state,32);
	IOReturn ret = WMIDevice->evaluateObject(method, &res, (OSObject**)&number,1);
	
	number->release();
	return ret;
}


#pragma mark -
#pragma mark Event handling methods
#pragma mark -


void IOWMIController::enableEvent()
{
	DbgLog("%s: IOWMIController::enableEvent()\n", this->getName());
}


void IOWMIController::disableEvent()
{
	if (_keyboardDevice)
	{
		_keyboardDevice->release();
		_keyboardDevice = NULL;
	}
}

