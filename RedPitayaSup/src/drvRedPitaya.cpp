
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <iterator>

#include <cantProceed.h>
#include <epicsExit.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <errlog.h>
#include <iocsh.h>
#include <math.h>
#include <unistd.h>

#include <asynParamType.h>

#include "rp.h"
#include "drvRedPitaya.h"

// Useful type neutral numerical macro fuctions.
//
#define MIN(a, b)           ((a) <= (b) ? (a) : (b))

// Calculates number of items in an array
//
#define ARRAY_LENGTH(xx)   ((int) (sizeof (xx) /sizeof (xx [0])))

#define REDPITAYA_DRIVER_VERSION   "2.2"

#define MAX_NUMBER_OF_ADDRESSES    8      // 0 to 8


// Pin and led minimum and maximum values
//
#define MIN_DIG_PIN       0
#define MAX_DIG_PIN       7

#define N_PIN_OFFSET   16
#define P_PIN_OFFSET   8

// Initialisation of decimation factors
//

struct QualifierDefinitions {
   asynParamType type;
   const char* name;
};

// Qualifier lookup table - used by RedPitaya_DrvUser_Create and
// RedPitaya_Qualifier_Image
//
// MUST be consistent with enum Qualifiers type out of RedPitayaDriver (in drvRedPitaya.h)
//
static const QualifierDefinitions qualifierList[] = {
      // General info
      //
      { asynParamOctet,        "DRVVER"      },  // DriverVersion

      // Digital pins
      //
      { asynParamInt32,        "NDPDIR"      },  // Set Digital Pin N to be an Input or Output
      { asynParamInt32,        "PDPDIR"      },  // Set Digital Pin P to be an Input or Output
      { asynParamInt32,        "NDPSTATE"    },  // Digital Pin N State
      { asynParamInt32,        "PDPSTATE"    }  // Digital Pin P State

   };

// Supported interrupts.
//
const static int interruptMask = asynFloat32ArrayMask;

// Any interrupt must also have an interface.
//
const static int interfaceMask = interruptMask | asynDrvUserMask | asynOctetMask
      | asynInt32Mask | asynFloat64Mask | asynFloat32ArrayMask;

const static int asynFlags = ASYN_MULTIDEVICE | ASYN_CANBLOCK;

// Static data.
//
static int verbosity = 4;               // High until set low
static int driver_initialised = false;
static bool singleShot;

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
//
static void devprintf(const int required_min_verbosity, const char* function,
      const int line_no, const char* format, ...) {
   if (verbosity >= required_min_verbosity) {
      char message[100];
      va_list arguments;
      va_start(arguments, format);
      vsnprintf(message, sizeof(message), format, arguments);
      va_end(arguments);
      errlogPrintf("RedPitayaDriver: %s:%d  %s", function, line_no, message);
   }
}

// Wrapper macros to devprintf.
//
#define ERROR(...)    devprintf (0, __FUNCTION__, __LINE__, __VA_ARGS__);
#define WARNING(...)  devprintf (1, __FUNCTION__, __LINE__, __VA_ARGS__);
#define INFO(...)     devprintf (2, __FUNCTION__, __LINE__, __VA_ARGS__);
#define DETAIL(...)   devprintf (3, __FUNCTION__, __LINE__, __VA_ARGS__);

//==============================================================================
//
//==============================================================================
//
const char* RedPitayaDriver::qualifierImage(const Qualifiers q) {
   static char result[24];

   if ((q >= 0) && (q < NUMBER_QUALIFIERS)) {
      return qualifierList[q].name;
   } else {
      sprintf(result, "unknown (%d)", q);
      return result;
   }
}

//------------------------------------------------------------------------------
// static
//
static void RedPitaya_Initialise(const int verbosityIn) {
   verbosity = verbosityIn;
   driver_initialised = true;
}

//------------------------------------------------------------------------------
//
void RedPitayaDriver::shutdown(void* arg) {
   RedPitayaDriver* self = (RedPitayaDriver*) arg;

   if (self && self->is_initialised) {
      INFO("RedPitayaDriver: shutting down: %s\n", self->full_name)
      // If we don't reset the FPGA continues on doing whatever
      // it has been doing
      //
      rp_Reset();
      rp_Release();
   }
}

//

//------------------------------------------------------------------------------
//
RedPitayaDriver::RedPitayaDriver(const char* port_name, const double pollingInterval) :
      asynPortDriver(port_name,              //
                     MAX_NUMBER_OF_ADDRESSES,//
                     NUMBER_QUALIFIERS,      //
                     interfaceMask,          //
                     interruptMask,          //
                     asynFlags,              //
                     1,                      // Auto-connect
                     0,                      // Default priority
                     0)                      // Default stack size

{

   this->is_initialised = false;   // flag object as not initialised.

   if (!driver_initialised) {
      ERROR("driver not initialised (call %s first)\n", "RedPitaya_Initialise");
      return;
   }

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   // Verify port names are sensible
   //
   if ((port_name == NULL) || (strcmp(port_name, "") == 0)) {
      ERROR("null/empty port name\n", 0);
      return;
   }

   snprintf(this->full_name, sizeof(this->full_name), "%s", port_name);

   if (pollingInterval < 0.0) {
      ERROR("Trigger polling interval needs to be a positive number\n", 0);
      return;
   }


   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // Set up asyn parameters.
   //
   for (int j = 0; j < ARRAY_LENGTH(qualifierList); j++) {
      asynStatus status = createParam(qualifierList[j].name,
            qualifierList[j].type, &this->indexList[j]);
      if (status != asynSuccess) {
         ERROR("Parameter creation failed\n");
         return;
      }
   }

   int initStatus = rp_Init();
   if (initStatus != RP_OK) {
      ERROR("Driver initialization failed with error: %s\n",
            rp_GetError(initStatus));
      return;
   }

   INFO("RedPitaya library version: %s\n", rp_GetVersion());

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // Register for epics exit callback.
   // Note: shutdown is a static function so we must pass this as a parameter.
   //
   epicsAtExit(RedPitayaDriver::shutdown, this);

   this->is_initialised = true;

   INFO("%s initialisation complete\n", this->full_name);
}

RedPitayaDriver::~RedPitayaDriver() {
   // Clean up a bit
}

//------------------------------------------------------------------------------
// Asyn callback functions
//------------------------------------------------------------------------------
//
void RedPitayaDriver::report(FILE * fp, int details) {
   if (details > 0) {
      fprintf(fp, "    driver info:\n");
      fprintf(fp, "        initialised:  %s\n",
            this->is_initialised ? "yes" : "no");
      fprintf(fp, "\n");
   }
}

//-----------------------------------------------------------------------------
// A function that runs in a separate thread and reads the data.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
asynStatus RedPitayaDriver::readOctet(asynUser* pasynUser, char* data,
      size_t maxchars, size_t* nbytesTransfered, int* eomReason) {
   const Qualifiers qualifier = static_cast<Qualifiers> (pasynUser->reason);
   asynStatus status;
   int addr;
   size_t length;

   *nbytesTransfered = 0;

   // Extract and validate address index.
   //
   pasynManager->getAddr(pasynUser, &addr);

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

   case DriverVersion:
      strncpy(data, REDPITAYA_DRIVER_VERSION, maxchars);
      length = strlen(REDPITAYA_DRIVER_VERSION);
      *nbytesTransfered = MIN(length, maxchars);
      break;

   default:
      ERROR("%s Unexpected qualifier (%s)\n", this->full_name,
            qualifierImage(qualifier))
      ;
      status = asynError;
      break;
   }

   return status;
}


//------------------------------------------------------------------------------
//
asynStatus RedPitayaDriver::writeInt32(asynUser* pasynUser, epicsInt32 value) {
   const Qualifiers qualifier = static_cast<Qualifiers> (pasynUser->reason);
   asynStatus status;
   int addr;

   if (this->is_initialised != true) {
      return asynError;
   }

   // Extract and validate address index.
   //
   pasynManager->getAddr(pasynUser, &addr);

   status = asynSuccess;

   if (status != asynSuccess) {
      return status;
   }

   status = asynSuccess;       // hypothesise okay
   int rpStatus = 0;           // hypothesise okay

   switch (qualifier) {

   case PDigPinDir:
      rpStatus = rp_DpinSetDirection(rp_dpin_t(P_PIN_OFFSET + addr), rp_pinDirection_t(value));
      break;
   case NDigPinDir:
      rpStatus = rp_DpinSetDirection(rp_dpin_t(N_PIN_OFFSET + addr), rp_pinDirection_t(value));
      break;
   case PDigPinState:
      rpStatus = rp_DpinSetState(rp_dpin_t(P_PIN_OFFSET + addr), rp_pinState_t(value));
      break;
   case NDigPinState:
      rpStatus = rp_DpinSetState(rp_dpin_t(N_PIN_OFFSET + addr), rp_pinState_t(value));
      break;
   default:
      ERROR("%s Unexpected qualifier (%s)\n", this->full_name, qualifierImage(qualifier));
      status = asynError;
      break;
   }

   if (rpStatus != 0) {
      ERROR("%s Command for qualifier (%s) failed with message: %s\n", this->full_name, qualifierImage(qualifier), rp_GetError(rpStatus));
      status = asynError;
   }

   return status;
}

//------------------------------------------------------------------------------
//
asynStatus RedPitayaDriver::readInt32(asynUser* pasynUser, epicsInt32* value) {
   const Qualifiers qualifier = static_cast<Qualifiers> (pasynUser->reason);
   asynStatus status;
   int addr;

   if (this->is_initialised != true) {
      return asynError;
   }

   // Extract and validate address index.
   //
   pasynManager->getAddr(pasynUser, &addr);

   status = asynSuccess;

   if (status != asynSuccess) {
      return status;
   }

   status = asynSuccess;       // hypothesise okay
   int rpStatus = 0;           // hypothesise okay

   switch (qualifier) {
   case PDigPinDir: {
      rp_pinDirection_t direction;
      rpStatus = rp_DpinGetDirection(static_cast<rp_dpin_t> (P_PIN_OFFSET + addr), &direction);
      if (rpStatus == RP_OK) {
         *value = direction;
      }
   }
      break;
   case NDigPinDir: {
      rp_pinDirection_t direction;
      rpStatus = rp_DpinGetDirection(static_cast<rp_dpin_t> (N_PIN_OFFSET + addr), &direction);
      if (rpStatus == RP_OK) {
         *value = direction;
      }
   }
      break;
   case PDigPinState: {
      rp_pinState_t state;
      rpStatus = rp_DpinGetState(static_cast<rp_dpin_t> (P_PIN_OFFSET + addr), &state);
      if (rpStatus == RP_OK) {
         *value = state;
      }
   }
      break;
   case NDigPinState: {
      rp_pinState_t state;
      rpStatus = rp_DpinGetState(static_cast<rp_dpin_t> (N_PIN_OFFSET + addr), &state);
      if (rpStatus == RP_OK) {
         *value = state;
      }
   }
      break;
   default:
      ERROR("%s Unexpected qualifier (%s)\n", this->full_name, qualifierImage(qualifier))
      ;
      status = asynError;
      break;
   }

   if (rpStatus != RP_OK) {
      ERROR("%s Request for qualifier (%s) failed with message: %s\n", this->full_name, qualifierImage(qualifier), rp_GetError(rpStatus));
      status = asynError;
   }

   return status;

}
//------------------------------------------------------------------------------
// IOC shell command registration
//------------------------------------------------------------------------------
//

// Define argument kinds
//
static const iocshArg verbosity_arg =        { "Verbosity (0 .. 4)", iocshArgInt             };
static const iocshArg port_name_arg =        { "ASYN port name", iocshArgString              };
static const iocshArg polling_interval_arg = { "Trigger polling interval", iocshArgDouble    };

//------------------------------------------------------------------------------
//
static const iocshArg * const RedPitaya_Initialise_Args[1] = { &verbosity_arg, };

static const iocshFuncDef RedPitaya_Initialise_Func_Def = { "RedPitaya_Initialise", 1, RedPitaya_Initialise_Args };

static void Call_RedPitaya_Initialise(const iocshArgBuf* args) {
   RedPitaya_Initialise(args[0].ival);
}

//------------------------------------------------------------------------------
//
static const iocshArg * const RedPitaya_Configure_Args[2] = { &port_name_arg, &polling_interval_arg};

static const iocshFuncDef RedPitaya_Configure_Func_Def = { "RedPitaya_Configure", 2, RedPitaya_Configure_Args };

static void Call_RedPitaya_Configure(const iocshArgBuf* args) {
   new RedPitayaDriver(args[0].sval, args[1].dval);
}

//------------------------------------------------------------------------------
//
static void RedPitaya_Startup(void) {
   INFO("RedPitaya Startup (driver version %s)\n", REDPITAYA_DRIVER_VERSION);
   iocshRegister(&RedPitaya_Initialise_Func_Def, Call_RedPitaya_Initialise);
   iocshRegister(&RedPitaya_Configure_Func_Def, Call_RedPitaya_Configure);
}

epicsExportRegistrar (RedPitaya_Startup);

// end
