/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*       Solutions for real time microcontroller applications         *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2018 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* * This software may in its unmodified form be freely redistributed *
*   in source form.                                                  *
* * The source code may be modified, provided the source code        *
*   retains the above copyright notice, this list of conditions and  *
*   the following disclaimer.                                        *
* * Modified versions of this software in source or linkable form    *
*   may not be distributed without prior consent of SEGGER.          *
* * This software may only be used for communication with SEGGER     *
*   J-Link debug probes.                                             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
---------------------------END-OF-HEADER------------------------------
  Sample implementation for using J-Scope with RTT

  Generates 3 sine waveforms and sends data to J-Scope application
  using RTT. Available 	operating modes are:
  
  SYSTICK   real time calculation with floating point values.
            Output is converted to 8bit integer and sent to J-Scope
            using "RTT with timestamp".

  TIMERLESS in timerless mode, waveform data is permanently
            created using a lookup table. Output format is 32bit
            integer and sent to J-Scope using
            "RTT without timestamp"
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "SEGGER_RTT.h"

#define PI 3.14159265358979323846f

#define CLK_FREQ   40000000u   // clock frequency
#define TICK_FREQ     40000u   // tick frequency
#define TIMERLESS              // timerless mode / systick mode

#define SINUS_FREQ        50.f // frequency for sine wave generation
#define SINUS_SAMPLE_DATA  500 /* Number of Sinus samples per 90°; set multiplier in SysTick_Config (OS_PCLK_TIMER / (OS_TICK_FREQ * multiplier))  to (SINUS_SAMPLE_DATA / 5) in order to create a 50Hz Sinus*/
#define SINUS_SCALE      12000 /* SIN_MAX = sin(90) = 1 * SINUS_SCALE (PI/2*SINUS_SCALE)*/

char JS_RTT_UpBuffer[4096];    // J-Scope RTT Buffer
int  JS_RTT_Channel = 1;       // J-Scope RTT Channel

/* Static sinus lookup table */ 
static unsigned _aSin[SINUS_SAMPLE_DATA];
static unsigned _SinAShift;
static unsigned _SinBShift;
static unsigned _SinCShift;

unsigned us;                   // microseconds since start
unsigned tb;                   // microseconds time base (60s cycle)

int Sine1;                     // sine wave 1 (global for HSS access
int Sine2;                     // sine wave 2 (global for HSS access
int Sine3;                     // sine wave 3 (global for HSS access
float t;                       // time base for calculation (in seconds)

/*********************************************************************
*
*       _CalcSinusData()
*
* Function description
*   Fills _aSin[SINUS_SAMPLE_DATA] with data values 
*  
*/
static void _CalcSinusData(void) {
  unsigned i;
  for (i = 0; i < SINUS_SAMPLE_DATA; i++) {
    _aSin[i] = (sin((float)i * PI / (2. * SINUS_SAMPLE_DATA)) * SINUS_SCALE); // calcs sin data for 0-90° (0-PI/2)
  }
  _SinAShift = SINUS_SAMPLE_DATA * 0.00000f;
  _SinBShift = SINUS_SAMPLE_DATA * 1.33333f;
  _SinCShift = SINUS_SAMPLE_DATA * 2.66667f;
}

/*********************************************************************
*
*       _GetSinVal()
*
* Function description
*   Returns the corresponding y-value for the x-Value passed
*
*  Parameter
*    XVal - 0 <= XVal < (4 * SINUS_SAMPLE_DATA)
*/
int _GetSinVal(unsigned XVal) {
  switch (XVal / SINUS_SAMPLE_DATA) {
  case 0:
    return _aSin[XVal];                                          /*Values 0°-90°    = "normal"*/
  case 1:
    return _aSin[(((2 * SINUS_SAMPLE_DATA) - 1) - XVal)];        /*Values 90°-180°  = 0°-90° in reverse order (=> 89° <=> 91°; 30° <=> 150° etc)*/
  case 2:
    return _aSin[XVal - (2 * SINUS_SAMPLE_DATA)] * (-1);         /*Values 180°-270° = 0-90° inverted*/
  case 3:
    return _aSin[(((4 * SINUS_SAMPLE_DATA) - 1) - XVal)] * (-1); /*Values 270°-360° = 90°-180° inverted*/
  default:
    return 0;
  }
}

void SineLUT(void) {
  //
  // RTT block structure
  //
  #pragma pack(push, 1)
  struct {
    signed int Sine1;
    signed int Sine2;
    signed int Sine3;
  } acValBuffer;
  #pragma pack(pop)

  static unsigned XCnt;

  Sine1 = _GetSinVal((XCnt + _SinAShift) % (4 * SINUS_SAMPLE_DATA));
  Sine2 = _GetSinVal((XCnt + _SinBShift) % (4 * SINUS_SAMPLE_DATA));
  Sine3 = _GetSinVal((XCnt + _SinCShift) % (4 * SINUS_SAMPLE_DATA));

  //
  // send over RTT channel
  //
  acValBuffer.Sine1 = Sine1;
  acValBuffer.Sine2 = Sine2;
  acValBuffer.Sine3 = Sine3;

  SEGGER_RTT_Write(JS_RTT_Channel, &acValBuffer, sizeof(acValBuffer));

  XCnt++;
}

void SineFLT(void) {
  //
  // RTT block structure
  //
  #pragma pack(push, 1)
  struct {
    unsigned t;
    signed char Sine1;
    signed char Sine2;
    signed char Sine3;
  } acValBuffer;
  #pragma pack(pop)

  //
  // calculate time base
  //
  us += 1000000u / TICK_FREQ;  // calculate microseconds (total)
  tb += 1000000u / TICK_FREQ;  // calculate microseconds (time base)

  if (tb > 60000000u) {        // 60s cycle
    tb -= 60000000u;
  }
  t = tb / 1E6f;               // time base in seconds

  //
  // calculate waveforms
  //
  Sine1 = floor(100.f * sinf(2.f * PI * SINUS_FREQ * (t + 0.f / 3.f)));
  Sine2 = floor(100.f * sinf(2.f * PI * SINUS_FREQ * (t + 2.f / 3.f)));
  Sine3 = floor(100.f * sinf(2.f * PI * SINUS_FREQ * (t + 4.f / 3.f)));

  //
  // send over RTT channel
  //
  acValBuffer.t = us;
  acValBuffer.Sine1 = Sine1;
  acValBuffer.Sine2 = Sine2;
  acValBuffer.Sine3 = Sine3;

  SEGGER_RTT_Write(JS_RTT_Channel, &acValBuffer, sizeof(acValBuffer));
}

void SysTick_Handler() {
  SineFLT();
}

void main(void) {
#ifdef SYSTICK
  //
  // Systick mode (requires CMSIS library)
  //
  SEGGER_RTT_ConfigUpBuffer(JS_RTT_Channel, "JScope_T4I1I1I1", &JS_RTT_UpBuffer[0], sizeof(JS_RTT_UpBuffer), SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  SysTick_Config(CLK_FREQ / TICK_FREQ);
  while (1);
#endif
#ifdef TIMERLESS
  //
  // Timerless mode
  //
  SEGGER_RTT_ConfigUpBuffer(JS_RTT_Channel, "JScope_I4I4I4", &JS_RTT_UpBuffer[0], sizeof(JS_RTT_UpBuffer), SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  _CalcSinusData(); // init sinus lookup table
  while (1) {
    SineLUT();
  }
#endif
}
