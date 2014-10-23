#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inpfile.h"


// convience def parameters
#define MAXLINE 256
#define SEPSTR " \t\n\r"  /* Token separator characters */

#define   MAXTOKS   40       /* Max. items per line of input           */

#define TO_UPPER(x) (((x) >= 'a' && (x) <= 'z') ? ((x)&~32) : (x))


/* ---------Input Section Names ---------- */
#define   s_TITLE       "[TITL"
#define   s_JUNCTIONS   "[JUNC"
#define   s_RESERVOIRS  "[RESE"
#define   s_TANKS       "[TANK"
#define   s_PIPES       "[PIPE"
#define   s_PUMPS       "[PUMP"
#define   s_VALVES      "[VALV"
#define   s_CONTROLS    "[CONT"
#define   s_RULES       "[RULE"
#define   s_DEMANDS     "[DEMA"
#define   s_SOURCES     "[SOUR"
#define   s_EMITTERS    "[EMIT"
#define   s_PATTERNS    "[PATT"
#define   s_CURVES      "[CURV"
#define   s_QUALITY     "[QUAL"
#define   s_STATUS      "[STAT"
#define   s_ROUGHNESS   "[ROUG"
#define   s_ENERGY      "[ENER"
#define   s_REACTIONS   "[REAC"
#define   s_MIXING      "[MIXI"
#define   s_REPORT      "[REPO"
#define   s_TIMES       "[TIME"
#define   s_OPTIONS     "[OPTI"
#define   s_COORDS      "[COOR"
#define   s_VERTICES    "[VERT"
#define   s_LABELS      "[LABE"
#define   s_BACKDROP    "[BACK"
#define   s_TAGS        "[TAGS"
#define   s_END         "[END"




// private types

typedef struct {
  size_t junctions, tanks,
         pipes, pumps, valves,
         controls, rules, curves,
         patterns,
         coords;
  
  long *sectionOffsets; // dynamically allocated to sectionType_END size.
  
} netsize_t;


typedef enum {
  sectionType_TITLE,
  sectionType_JUNCTIONS,
  sectionType_RESERVOIRS,
  sectionType_TANKS,
  sectionType_PIPES,
  sectionType_PUMPS,
  sectionType_VALVES,
  sectionType_CONTROLS,
  sectionType_RULES,
  sectionType_DEMANDS,
  sectionType_SOURCES,
  sectionType_EMITTERS,
  sectionType_PATTERNS,
  sectionType_CURVES,
  sectionType_QUALITY,
  sectionType_STATUS,
  sectionType_ROUGHNESS,
  sectionType_ENERGY,
  sectionType_REACTIONS,
  sectionType_MIXING,
  sectionType_REPORT,
  sectionType_TIMES,
  sectionType_OPTIONS,
  sectionType_COORDS,
  sectionType_VERTICES,
  sectionType_LABELS,
  sectionType_BACKDROP,
  sectionType_TAGS,
  sectionType_END
} section_t;

char *SectionText[] = { s_TITLE,     s_JUNCTIONS, s_RESERVOIRS,
                        s_TANKS,     s_PIPES,     s_PUMPS,
                        s_VALVES,    s_CONTROLS,  s_RULES,
                        s_DEMANDS,   s_SOURCES,   s_EMITTERS,
                        s_PATTERNS,  s_CURVES,    s_QUALITY,
                        s_STATUS,    s_ROUGHNESS, s_ENERGY,
                        s_REACTIONS, s_MIXING,    s_REPORT,
                        s_TIMES,     s_OPTIONS,   s_COORDS,
                        s_VERTICES,  s_LABELS,    s_BACKDROP,
                        s_TAGS,      s_END,
                        NULL };



typedef struct {
  size_t count;
  char **tokens;
} lineToken_t;



// private methods
int _netsize(FILE *inpFile, netsize_t *netSize);
int _findmatch(char *line, char *keyword[]);
int _match(char *str, char *substr);
int _scanFileToSection(FILE *handle, section_t section, netsize_t netsize);

int _getPatterns(OW_Project *modelObj, FILE *file);
int  _gettokens(char *s, char** Tok, int maxToks);

lineToken_t _newTokensFromLine(FILE *file);
int _freeTokens(lineToken_t line);



int DLLEXPORT OW_loadInpFile(char *inpFile, OW_Project **modelObj)
{
  int err;
  
  
  // get network size struct
  // construct junctions, tanks, reservoirs
  
  netsize_t theNetSize;
  
  
  // attempt to open the file
  FILE *inpFileHandle;
  if ((inpFileHandle = fopen(inpFile,"rt")) == NULL)
  {
    return OW_ERR_CANT_OPEN_FILE;
  }
  
  err = _netsize(inpFileHandle, &theNetSize);
  if (err != OW_OK) {
    return err;
  }
  
  
  // get file handle to patterns section
  _scanFileToSection(inpFileHandle, sectionType_PATTERNS, theNetSize);
  // fetch patterns
  _getPatterns(*modelObj, inpFileHandle);
  
  
  // scan to [title] section
  // get title from file
  // set title with api
  
  // scan to [junctions]
  // while in junctions section
  //  - get junction info
  //  - use api to create junction
  // [same for tanks, reservoirs]
  
  // scan to [pipes] section
  // while in pipes section
  //  - get pipe info
  //  - use api to create pipes
  // [same for pumps, valves]
  
  
  
  
  
  
  
  
  return EN_OK;
}



int scanToSection(FILE *file, section_t section)
{
  return OW_OK;
}





int _netsize(FILE *inpFile, netsize_t *netSize)
/*
 **--------------------------------------------------------------
 **  Input:   none
 **  Output:  returns error code
 **  Purpose: determines number of system components
 **--------------------------------------------------------------
 */
{
  
  char  line[MAXLINE+1];     /* Line from input data file    */
  char  *tok;                /* First token of line          */
  int   sect = sectionType_END, newsect = sectionType_END;        /* Input data sections          */
  int   errcode = 0;         /* Error code                   */
  
  
  
  
  
  // clear out offsets
  int i = 0;
  netSize->sectionOffsets = calloc(sectionType_END, sizeof(long));
  
  for (; i < sectionType_END; ++i) {
    // signify that the section does not exist.
    // just in case it's not in the file.
    netSize->sectionOffsets[i] = -1;
  }
  
  
  netSize->junctions = 0;
  netSize->tanks = 0;
  netSize->pipes = 0;
  netSize->pumps = 0;
  netSize->valves = 0;
  netSize->controls = 0;
  netSize->rules = 0;
  netSize->curves = 0;
  netSize->patterns = 0;
  netSize->coords = 0;

  /* Make pass through data file counting number of each component */
  while (fgets(line, MAXLINE, inpFile) != NULL)
  {
    /* Skip blank lines & those beginning with a comment */
    tok = strtok(line,SEPSTR);
    if (tok == NULL) continue;
    if (*tok == ';') continue;
    
    /* Check if line begins with a new section heading */
    if (*tok == '[')
    {
      newsect = _findmatch(tok,SectionText);
      if (newsect >= 0)
      {
        // yay, a new section.
        // save the offset in our netsize struct
        sect = newsect;
        netSize->sectionOffsets[sect] = ftell(inpFile);
        if (sect == sectionType_END) break;
        continue;
      }
      else continue;
    }
    
    /* Add to count of current component */
    switch(sect)
    {
      case sectionType_JUNCTIONS:
        netSize->junctions++;
        break;
      case sectionType_RESERVOIRS:
      case sectionType_TANKS:
        netSize->tanks++;
        break;
      case sectionType_PIPES:
        netSize->pipes++;
        break;
      case sectionType_PUMPS:
        netSize->pumps++;
        break;
      case sectionType_VALVES:
        netSize->valves++;
        break;
      case sectionType_CONTROLS:
        netSize->controls++;
        break;
      case sectionType_RULES:
        netSize->rules++;
        break; /* See RULES.C */
      case sectionType_PATTERNS:
        netSize->patterns++;
        break;
      case sectionType_CURVES:
        netSize->curves++;
        break;
      case sectionType_COORDS:
        break;
    }
    if (errcode) break;
  }
  
  return OW_OK;
  
}




int  _findmatch(char *line, char *keyword[])
/*
 **--------------------------------------------------------------
 **  Input:   *line      = line from input file
 **           *keyword[] = list of NULL terminated keywords
 **  Output:  returns index of matching keyword or
 **           -1 if no match found
 **  Purpose: determines which keyword appears on input line
 **--------------------------------------------------------------
 */
{
  int i = 0;
  while (keyword[i] != NULL)
  {
    if (_match(line,keyword[i])) return(i);
    i++;
  }
  return(-1);
}                        /* end of findmatch */

int  _match(char *str, char *substr)
/*
 **--------------------------------------------------------------
 **  Input:   *str    = string being searched
 **           *substr = substring being searched for
 **  Output:  returns 1 if substr found in str, 0 if not
 **  Purpose: sees if substr matches any part of str
 **
 **      (Not case sensitive)
 **--------------------------------------------------------------
 */
{
  int i,j;
  
  /*** Updated 9/7/00 ***/
  /* Fail if substring is empty */
  if (!substr[0]) return(0);
  
  /* Skip leading blanks of str. */
  for (i=0; str[i]; i++)
    if (str[i] != ' ') break;
  
  /* Check if substr matches remainder of str. */
  for (i=i,j=0; substr[j]; i++,j++)
    if (!str[i] || TO_UPPER(str[i]) != TO_UPPER(substr[j]))
      return(0);
  return(1);
}


int _scanFileToSection(FILE *handle, section_t section, netsize_t netsize)
{
  long offset = netsize.sectionOffsets[section];
  int err = fseek(handle, offset, SEEK_SET);
  
  if (err == 0) {
    return 0;
  }
  
  
  else {
    return OW_ERR_FILE_SEEK;
  }
  
  
}



int  _gettokens(char *s, char** Tok, int maxToks)
/*
 **--------------------------------------------------------------
 **  Input:   *s = string to be tokenized
 **  Output:  returns number of tokens in s
 **  Purpose: scans string for tokens, saving pointers to them
 **           in module global variable Tok[]
 **
 ** Tokens can be separated by the characters listed in SEPSTR
 ** (spaces, tabs, newline, carriage return) which is defined
 ** in TYPES.H. Text between quotes is treated as a single token.
 **--------------------------------------------------------------
 */
{
  int  len, m, n;
  char *c;
  
  /* Begin with no tokens */
  for (n=0; n<maxToks; n++) Tok[n] = NULL;
  n = 0;
  
  /* Truncate s at start of comment */
  c = strchr(s,';');
  if (c) *c = '\0';
  len = (int)strlen(s);
  
  /* Scan s for tokens until nothing left */
  while (len > 0 && n < MAXTOKS)
  {
    m = (int)strcspn(s,SEPSTR);          /* Find token length */
    len -= m+1;                     /* Update length of s */
    if (m == 0) s++;                /* No token found */
    else
    {
      if (*s == '"')               /* Token begins with quote */
      {
        s++;                      /* Start token after quote */
        m = (int)strcspn(s,"\"\n\r");  /* Find end quote (or EOL) */
      }
      s[m] = '\0';                 /* Null-terminate the token */
      Tok[n] = s;                  /* Save pointer to token */
      n++;                         /* Update token count */
      s += m+1;                    /* Begin next token */
    }
  }
  return(n);
}                        /* End of gettokens */



int _getPatterns(OW_Project *modelObj, FILE *file)
{
  
  
  
  
  return OW_OK;
}



