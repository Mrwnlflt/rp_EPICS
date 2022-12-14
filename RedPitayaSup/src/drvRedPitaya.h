/* Description:
 * This is an ASYN port driver to be used with an on board IOC for
 * RedPitaya.
 *
 * Copyright (c) 2018  Australian Synchrotron
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contact details:
 * andraz.pozar@synchrotron.org.au
 * 800 Blackburn Road, Clayton, Victoria 3168, Australia.
 */

#ifndef DRV_REDPITAYA_H_
#define DRV_REDPITAYA_H_

#include <epicsTime.h>
#include <epicsTypes.h>
#include <epicsThread.h>
#include <asynPortDriver.h>

class epicsShareFunc RedPitayaDriver : public asynPortDriver {
public:
   explicit RedPitayaDriver (const char* port_name, const double pollingInterval = 0.01);
   ~RedPitayaDriver ();

   /**
    * Enum which represents all supported driver actions. This is used to distinguish which
    * action should be executed. The order of qualifiers here has is directly mapped to the
    * qualifierList which contains list of commands.
    */
   enum Qualifiers {
     // General info
     //
     DriverVersion = 0,       // EPICS driver version

     // Digital pins and LEDs
     //
     NDigPinDir,              // Direction of N digital pins
     PDigPinDir,              // Direction of P digital pins
     NDigPinState,            // State of N digital pins
     PDigPinState,            // State of P digital pins
     NUMBER_QUALIFIERS        // Number of qualifiers - MUST be last
   };

   // Override asynPortDriver functions needed for this driver.
   //
   void report (FILE* fp, int details);
   asynStatus readOctet (asynUser* pasynUser, char* value, size_t maxChars,
                         size_t* nActual, int* eomReason);

   // A function that runs in a separate thread and is acquiring the data
   //
   asynStatus readInt32 (asynUser* pasynUser, epicsInt32* value);
   asynStatus writeInt32 (asynUser* pasynUser, epicsInt32 value);

private:
   static const char* qualifierImage (Qualifiers qualifer);
   static void shutdown (void* arg);


   /**
    * Checks if the address provided is within the limits for the given action.
    *
    * @param qualifier  The qualifier used to identify the action to be executed.
    * @param addr       Address provided from EPICS database record.
    *
    * @return           asynSuccess if the address is within bounds and asynError otherwise.
    */

   int indexList [NUMBER_QUALIFIERS];   // used by asynPortDriver
   char full_name [80];
   int is_initialised;                  // Device found, but initialisation failed.

};

#endif   // DRV_REDPITAYA_H_
