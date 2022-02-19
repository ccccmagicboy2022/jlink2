/*********************************************************************
*               SEGGER MICROCONTROLLER GmbH                          *
*       Solutions for real time microcontroller applications         *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2018  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       Current version number will be inserted here                 *
*       when shipment is built.                                      *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : NuttXPlugin.js
Purpose : Script for thread windows for NuttX
--------  END-OF-HEADER  ---------------------------------------------
*/

/*********************************************************************
*
*       Local Functions
*
**********************************************************************
*/

/*********************************************************************
*
*       AddList
*
* Function description
*   Adds all tasks of a task list to the task window
*
* Parameters
*   List:            task list (type xLIST)
*   CurrTaskAddr:    memory location of the executing task's control block (TCB)
*   sState:          common state of all tasks within 'list'
*/
function AddList(pList, sState) {
  var i;
  var context;
  var Item;
  var TaskAddr;

  i = 0;
  while(pList) {
    Item    = Debug.evaluate("*(tcb_s*)" + pList);
    name    = Debug.evaluate("(char*)(*(tcb_s*)" + pList + ").name");
    context = Debug.evaluate("&(*(tcb_s*)" + pList + ").xcp");

    Threads.add(name, Item.lockcount, Item.sched_priority, sState, Item.adj_stack_size, Item.pid, Item.group, context);

    if (i > 1000) { // infinite loop guard
      break;
    }
    i++;
    pList = Item.flink;
  }
}

/*********************************************************************
*
*       API Functions
*
**********************************************************************
*/

/*********************************************************************
*
*       init
*
* Function description
*   Initializes the task window
*/
function init() {
  Threads.clear();
  Threads.newqueue("Task List");
  Threads.setColumns("Name", "Lock Count", "Priority", "Status", "Stack Info", "PID", "Task Group");
  Threads.setColor("Status", "ready", "executing", "blocked");
}

/*********************************************************************
*
*       update
*
* Function description
*   Updates the task window
*/
function update() {
  var i;
  var pList;
  var List;
  var MaxPriority;
  var CurrTaskAddr;
  var tasklists = new Array("g_readytorun", "g_pendingtasks", "g_waitingforsemaphore", "g_waitingforsignal");
  var taskstates = new Array("ready", "pending", "blocked (sema)", "blocked (signal)");

  Threads.clear();

  if(Debug.evaluate("g_running_tasks[0]") == 0) {
    return;
  }

  for (i = 0; i < tasklists.length; i++) {
    pList = Debug.evaluate(tasklists[i] + ".head");
    if (pList != 0) {
      AddList(pList, taskstates[i]);
    }
  }
}

/*********************************************************************
*
*       getregs
*
* Function description
*   Returns the register set of a task.
*   For ARM cores, this function is expected to return the values
*   of registers R0 to R15 and PSR.
*
* Parameters
*   hTask: integer number identifiying the task.
*   Identical to the last parameter supplied to method Threads.add.
*   For convenience, this should be the address of the TCB.
*
* Return Values
*   An array of unsigned integers containing the task’s register values.
*   The array must be sorted according to the logical indexes of the regs.
*   The logical register indexing scheme is defined by the ELF-DWARF ABI.
*
**********************************************************************
*/
function getregs(hTask) {
  var i;
  var off;
  var xcp;
  var regs;
  var aRegs = new Array(17);

  xcp  = Debug.evaluate("*(xcptcontext*)" + hTask);
  regs = Debug.evaluate("sizeof(xcptcontext.regs)");

  if (regs == 212) {
    off = 16;
  } else {
    off = 0;
  }

  //
  // R4...R11
  //
  for (i = 4; i < 12; i++) {
    aRegs[i] = xcp.regs[i - 2];
  }
  //
  // S16...S31
  //
  if ((aRegs[12] & 0x10) != 0x10) { // FP context has been saved?
  }
  //
  // R0...R3
  //
  for (i = 0; i < 4; i++) {
    aRegs[i] = xcp.regs[i + 11 + off];
  }
  //
  // R12, SP, LR, PC, PSR
  //
  aRegs[12] = xcp.regs[15 + off];
  aRegs[13] = xcp.regs[0];
  aRegs[14] = xcp.regs[16 + off];
  aRegs[15] = xcp.regs[16 + off]; // Send LR as PC (improves call-stack handling)
  aRegs[16] = xcp.regs[18 + off];
  //
  // S0..S15
  //
  return aRegs;
}

/*********************************************************************
*
*       getContextSwitchAddrs
*
*  Functions description
*    Returns an unsigned integer array containing the base addresses
*    of all functions that complete a task switch when executed.
*/
function getContextSwitchAddrs() {
  return [];
}

/*********************************************************************
*
*       getOSName()
*
*  Functions description:
*    Returns the name of the RTOS this script supplies support for
*/
function getOSName() {
  return "NuttX";
}

