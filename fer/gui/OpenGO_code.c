/*
*
*  This software was developed by the Thermal Modeling and Analysis
*  Project(TMAP) of the National Oceanographic and Atmospheric
*  Administration's (NOAA) Pacific Marine Environmental Lab(PMEL),
*  hereafter referred to as NOAA/PMEL/TMAP.
*
*  Access and use of this software shall impose the following
*  obligations and understandings on the user. The user is granted the
*  right, without any fee or cost, to use, copy, modify, alter, enhance
*  and distribute this software, and any derivative works thereof, and
*  its supporting documentation for any purpose whatsoever, provided
*  that this entire notice appears in all copies of the software,
*  derivative works and supporting documentation.  Further, the user
*  agrees to credit NOAA/PMEL/TMAP in any publications that result from
*  the use of this software or in any product that includes this
*  software. The names TMAP, NOAA and/or PMEL, however, may not be used
*  in any advertising or publicity to endorse or promote any products
*  or commercial entity unless specific written permission is obtained
*  from NOAA/PMEL/TMAP. The user also understands that NOAA/PMEL/TMAP
*  is not obligated to provide the user with any support, consulting,
*  training or assistance of any kind with regard to the use, operation
*  and performance of this software nor to provide the user with any
*  updates, revisions, new versions or "bug fixes".
*
*  THIS SOFTWARE IS PROVIDED BY NOAA/PMEL/TMAP "AS IS" AND ANY EXPRESS
*  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL NOAA/PMEL/TMAP BE LIABLE FOR ANY SPECIAL,
*  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
*  CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN
*  CONNECTION WITH THE ACCESS, USE OR PERFORMANCE OF THIS SOFTWARE.  
*
*/



/* 
 * OpenGO_code.c
 *
 * John Osborne,
 * Jonathan Callahan
 * Nov 25th 1996
 *
 * This file contains the auxiliary functions which are included by
 * OpenGO.c.
 *
 * 96.12.12 Removed JC_II_NewDataset from GOOpenOK().  All synching happens in ferret_command.
 */

/* .................... Function Definitions .................... */

static void ActivateCB(wid, clientData, cbArg)
Widget wid;
XtPointer clientData, cbArg;
{
	char *valText = XmTextFieldGetString(wid);

	if (strlen(valText))
		strcpy(GOText, valText);
	else {
		XBell(XtDisplay(UxGetWidget(FerretMainWd)), 50);
		XmTextSetString(wid, GOText);
	}
}

static char *CollectToSpace(targetStr, subStr)
char *targetStr, *subStr;
{
        while ((*targetStr != ' ') && (*targetStr != 0))
                *subStr++ = *targetStr++;
        *subStr++ = '\0';
        if (*targetStr != 0)
                return(++targetStr);
        else
                return(targetStr);
}

static void ViewCB()
{
	char *viewText = (char *)XtMalloc(10000), pathName[80], *pathText=(char *)XtMalloc(1000);
	char *path_ptr;
	FILE *fpin=NULL;
	int io;

	strcpy(pathText, getenv("FER_GO"));
	path_ptr = pathText;

	while (strlen(path_ptr = CollectToSpace(path_ptr, pathName)) &&
	       fpin == NULL) {
		sprintf(pathName, "%s/%s.jnl", pathName, GOText);
		fpin = fopen(pathName, "r");
	}

	io = fread(viewText, sizeof(char), 10000, fpin);

	/* close file */
	io = fclose(fpin); 

#ifdef NO_ENTRY_NAME_UNDERSCORES
	ferret_list_in_window(viewText, 0);
#else
	ferret_list_in_window_(viewText, 0);
#endif

	XtFree(viewText); /* allocated with XtMalloc() */
	XtFree(pathText); /* allocated with XtMalloc() */
}

void InitialList()
{
/* *kob* 3/99 upped cmd, goText, go and path to MAX_NAME_LENGTH - 
           a precaution */
	char goText[MAX_NAME_LENGTH], cmd[MAX_NAME_LENGTH];
	char go[MAX_NAME_LENGTH], path[MAX_NAME_LENGTH];
	char *envText=(char *)XtMalloc(1000);
	char *env_ptr;
	FILE *fpin;
	XmString xgoText;
	register int i, c, pos;

	/* isolate the individual paths in FER_GO */
	strcpy(envText, getenv("FER_GO"));
	env_ptr = envText;

	/* fill the list by reading a directory and finding *.jnl files */
	while (strlen(env_ptr)) {
		env_ptr = CollectToSpace(env_ptr, path);
		sprintf(cmd, "ls -1 %s/*.jnl", path);
		fpin = (FILE *)popen(cmd, "r");
		if (fpin == NULL) {
			perror("OpenGO_code.c");
			return;
		}
		while (fgets(goText, 80, fpin) != NULL) {
			/* check to see if this is a jnl file */
			if (!strstr(goText, ".jnl")) continue;
			if (isupper(goText[0])) continue;
			/* isolate file from path */
			for (i=strlen(goText); i>=0; i--) {
				if (goText[i-1] == '/') {
					pos = i;
				break;
				}
			}

			c = 0;
			for (i=pos; i<strlen(goText); i++)
				go[c++] = goText[i];
			go[c-5] = '\0';
		
			xgoText = XmStringCreateSimple(go);
	 		XmListAddItem(UxGetWidget(scrolledList3), xgoText, 0);
		}
		pclose(fpin);
	}

	/* init some other stuff */
	XmListSelectPos(UxGetWidget(scrolledList3), 1, TRUE);
	MaintainBtns();
	XtFree(envText); /* allocated with XtMalloc() */
}

void MaintainBtns()
{
	if (strlen(GOText)) {
		XtSetSensitive(pushButton19, TRUE);
		XtSetSensitive(pushButton25, TRUE);
	}
	else {
		XtSetSensitive(pushButton19, FALSE);
		XtSetSensitive(pushButton25, FALSE);
	}
}

void ListBrowserCB(wid, client_data, cbs)
Widget wid;
XtPointer client_data;
XmListCallbackStruct *cbs;
{
	char *tempText;
	
	strcpy(GOText, "");

	/* get text selection from list */
	XmStringGetLtoR(cbs->item, XmSTRING_DEFAULT_CHARSET, &tempText);

	/* construct GOText */
	strcpy(GOText, tempText);

	/* put this into edit text field */
	XmTextFieldSetString(UxGetWidget(textField14), GOText);
	
	MaintainBtns();
	XtFree(tempText); /* allocated with XmStringGetLtoR() */
}

void GOCancelOpen()
{
	/* dismiss the file selection box */
  	XtDestroyWidget(UxGetWidget(OpenGO));
}

void GOOpenOK()
{
  /* 	upped cmd from 80 to 256 chars - was causing crashes w/ dods datasets
        *kob* 3/25/99  and use macro MAX_NAME_LENGTH */
	char cmd[MAX_NAME_LENGTH];
	
	strcpy(cmd, "");
	sprintf(cmd, "GO %s", GOText);

	/* send go cmd to ferret */
	ferret_command(cmd, IGNORE_COMMAND_WIDGET);

}

