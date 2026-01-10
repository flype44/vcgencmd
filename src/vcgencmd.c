/********************************************************************
 * 
 * Program:  vcgencmd.c
 * Purpose:  github.com/raspberry/utils/blob/master/vcgencmd
 * Authors:  Philippe CARPENTIER
 * Target:   AmigaOS 3.x
 * Compiler: SAS/C Amiga Compiler 6.59
 * 
 ********************************************************************/

#include <dos/dos.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/mailbox.h>

#include "utils.h"
#include "vcgencmd.h"

/******************************************************************************
 * 
 * Defines
 * 
 ******************************************************************************/

#define MAILBOXNAME "mailbox.resource"

#define TEMPLATE "CMD=COMMAND/F/A"

typedef enum {
	OPT_COMMAND,
	OPT_COUNT
} OPT_ARGS;

/******************************************************************************
 * 
 * Globals
 * 
 ******************************************************************************/

APTR MailboxBase;

STRPTR VSTRING = VERSTRING;

extern struct ExecBase * SysBase;
extern struct DosLibrary * DOSBase;

/******************************************************************************
 * 
 * ShowUsage()
 * 
 ******************************************************************************/

static VOID ShowUsage(VOID) {
	PutStr("Usage: vcgencmd command [ params ]\n");
	PutStr("Send a command to the VideoCore and print the result.\n");
	PutStr("Without any argument this information is shown.\n");
	PutStr("Use the command 'vcgencmd commands' to get a list of available commands.\n");
	PutStr("Exit status 0 means command completed successfully");
	PutStr(" else VideoCore return an error\n");
	PutStr("For further documentation please see\n");
	PutStr("https://www.raspberrypi.com/documentation/computers/os.html#vcgencmd\n");
}

/******************************************************************************
 * 
 * ClearMem()
 * 
 ******************************************************************************/

static VOID ClearMem(ULONG * mem, ULONG size) {
	ULONG i;
	for (i = 0; i < size; i++) {
		mem[i] = 0x00000000;
	}
}

/******************************************************************************
 * 
 * SwapMem()
 * 
 ******************************************************************************/

static VOID SwapMem(ULONG * mem, ULONG size) {
	ULONG i;
	for (i = 0; i < size; i++) {
		mem[i] = LE32(mem[i]);
	}
}

/******************************************************************************
 * 
 * vcgencmd()
 * 
 ******************************************************************************/

#define BUFLEN (256)
#define BUFSIZE (1024)

static ULONG vcgencmd(STRPTR command, STRPTR reply, ULONG reply_capacity)
{
	ULONG i, retval, len = 0;
	ULONG FBReq[BUFLEN + 7];
	
	/* Command must be a valid pointer */
	if (command == NULL) {
		return -1;
	}
	
	/* Get command length */
	for (i = 0; i < BUFSIZE; i++) {
		if (command[i] == 0) {
			len = i - 1;
			break;
		}
	}
	
	/* Command length including \0 must not exceed 1024 bytes */
	if (len > 1023) {
		return -1;
	}
	
	/* Clear request */
	ClearMem(FBReq, BUFLEN + 7);
	
	/* Prepare request */
	FBReq[0] = 4 * (BUFLEN + 7);     // Request size in bytes
	FBReq[1] = 0;                    // Request code
	FBReq[2] = 0x00030080;           // Request tag
	FBReq[3] = BUFSIZE;              // Buffer size in bytes
	FBReq[4] = 0;                    // Result size in bytes
	FBReq[5] = 0;                    // Result code
	
	/* Copy command */
	CopyMem(command, &FBReq[6], len + 1);
	
	/* Send request */
	SwapMem(&FBReq[6], BUFLEN);
	MB_RawCommand(FBReq);
	SwapMem(&FBReq[6], BUFLEN);
	
	/* Copy reply */
	if (reply && reply_capacity > 0) {
		CopyMem(&FBReq[6], reply, reply_capacity > BUFSIZE ? 
			BUFSIZE : reply_capacity);
	}
	
	/* Result */
	if (FBReq[5] == 0x00000000) {
		retval = FBReq[5];
	} else {
		retval = -1;
	}
	
	return (retval);
}

/******************************************************************************
 * 
 * main()
 * 
 ******************************************************************************/

ULONG main(ULONG argc, STRPTR * argv)
{
	ULONG result;
	UBYTE reply[1024];
	LONG opts[OPT_COUNT];
	struct RDArgs * rdargs;
	
	// Open mailbox.resource
	if (!(MailboxBase = OpenResource(MAILBOXNAME))) {
		PutStr("Cant open " MAILBOXNAME "\n");
		return (RETURN_FAIL);
	}
	
	// Read arguments
	opts[OPT_COMMAND] = 0L;
	if (!(rdargs = (struct RDArgs *)ReadArgs(TEMPLATE, opts, NULL))) {
		ShowUsage();
		return (RETURN_OK);
	}
	FreeArgs(rdargs);
	
	// Execute command
	result = vcgencmd((STRPTR)opts[OPT_COMMAND], reply, 1024);
	PutStr(reply);
	PutStr("\n");
	
	// Return code
	return ((result == -1) ? RETURN_WARN : RETURN_OK);
}

/******************************************************************************
 * 
 * End of file
 * 
 ******************************************************************************/
