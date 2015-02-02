/* The CGI_C library, by Thomas Boutell, version 2.01. CGI_C is intended
	to be a high-quality API to simplify CGI programming tasks. */

/* Make sure this is only included once. */

#ifndef CGI_C
#define CGI_C 1

/* Bring in standard I/O since some of the functions refer to
	types defined by it, such as FILE *. */

//#include "fcgi_stdio.h"
#include <fcgiapp.h>
#include <stdio.h>

/* The various CGI environment variables. Instead of using getenv(),
	the programmer should refer to these, which are always
	valid null-terminated strings (they may be empty, but they 
	will never be null). If these variables are used instead
	of calling getenv(), then it will be possible to save
	and restore CGI environments, which is highly convenient
	for debugging. */


typedef struct cgiFormEntryStruct {
        char *attr;
    /* value is populated for regular form fields only.
        For file uploads, it points to an empty string, and file
        upload data should be read from the file tfileName. */ 
    char *value;
    /* When fileName is not an empty string, tfileName is not null,
        and 'value' points to an empty string. */
    /* Valid for both files and regular fields; does not include
        terminating null of regular fields. */
    int valueLength;
    char *fileName; 
    char *contentType;
    /* Temporary file name for working storage of file uploads. */
    char *tfileName;
        struct cgiFormEntryStruct *next;
} cgiFormEntry;




struct cgi_env {
int cgiRestored;// = 0;
char *cgiServerSoftware;
char *cgiServerName;
char *cgiGatewayInterface;
char *cgiServerProtocol;
char *cgiServerPort;
char *cgiRequestMethod;
char *cgiPathInfo;
char *cgiPathTranslated;
char *cgiScriptName;
char *cgiQueryString;
char *cgiRemoteHost;
char *cgiRemoteAddr;
char *cgiAuthType;
char *cgiRemoteUser;
char *cgiRemoteIdent;
char cgiContentTypeData[1024];
char *cgiContentType;// = cgiContentTypeData;

char * cgiMultipartBoundary;
cgiFormEntry *cgiFormEntryFirst;
int cgiTreatUrlEncoding;

char *cgiAccept;
char *cgiUserAgent;
char *cgiReferrer;

/* Cookies as sent to the server. You can also get them
	individually, or as a string array; see the documentation. */
char *cgiCookie;
char *cgiSid;

/* A macro providing the same incorrect spelling that is
	found in the HTTP/CGI specifications */
#define cgiReferer cgiReferrer

/* The number of bytes of data received.
	Note that if the submission is a form submission
	the library will read and parse all the information
	directly from cgiIn; the programmer need not do so. */

int cgiContentLength;

/* Pointer to CGI output. The cgiHeader functions should be used
	first to output the mime headers; the output HTML
	page, GIF image or other web document should then be written
	to cgiOut by the programmer. In the standard CGIC library,
	cgiOut is always equivalent to stdout. */

char *cgiFindTarget;
cgiFormEntry *cgiFindPos;

};
/* Possible return codes from the cgiForm family of functions (see below). */

typedef enum {
	cgiFormSuccess,
	cgiFormTruncated,
	cgiFormBadType,
	cgiFormEmpty,
	cgiFormNotFound,
	cgiFormConstrained,
	cgiFormNoSuchChoice,
	cgiFormMemory,
	cgiFormNoFileName,
	cgiFormNoContentType,
	cgiFormNotAFile,
	cgiFormOpenFailed,
	cgiFormIO,
	cgiFormEOF
} cgiFormResultType;

/* These functions are used to retrieve form data. See
	cgic.html for documentation. */

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormString(
	char *name, char *result, int max,struct cgi_env **);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormStringNoNewlines(
	char *name, char *result, int max,struct cgi_env ** ce);


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormStringSpaceNeeded(
	char *name, int *length,struct cgi_env ** ce);


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormStringMultiple(
	char *name, char ***ptrToStringArray,struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
void cgiStringArrayFree(char **stringArray);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormInteger(
	char *name, int *result, int defaultV,struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormIntegerBounded(
	char *name, int *result, int min, int max, int defaultV,struct cgi_env **ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormDouble(
	char *name, double *result, double defaultV,struct cgi_env **);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormDoubleBounded(
	char *name, double *result, double min, double max, double defaultV, struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormSelectSingle(
	char *name, char **choicesText, int choicesTotal, 
	int *result, int defaultV,struct cgi_env **ce);	


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormSelectMultiple(
	char *name, char **choicesText, int choicesTotal, 
	int *result, int *invalid,struct cgi_env **);

/* Just an alias; users have asked for this */
#define cgiFormSubmitClicked cgiFormCheckboxSingle

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormCheckboxSingle(
	char *name,struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormCheckboxMultiple(
	char *name, char **valuesText, int valuesTotal, 
	int *result, int *invalid,struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormRadio(
	char *name, char **valuesText, int valuesTotal, 
	int *result, int defaultV,struct cgi_env **ce);	

/* The paths returned by this function are the original names of files
	as reported by the uploading web browser and shoult NOT be
	blindly assumed to be "safe" names for server-side use! */
extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileName(
	char *name, char *result, int max,struct cgi_env **);

/* The content type of the uploaded file, as reported by the browser.
	It should NOT be assumed that browsers will never falsify
	such information. */
extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileContentType(
	char *name, char *result, int max,struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileSize(
	char *name, int *sizeP,struct cgi_env ** ce);

typedef struct cgiFileStruct *cgiFilePtr;

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileOpen(
	char *name, cgiFilePtr *cfpp,struct cgi_env ** ce);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileRead(
	cgiFilePtr cfp, char *buffer, int bufferSize, int *gotP);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileClose(
	cgiFilePtr cfp);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiCookieString(
	char *name, char *result, int max,char * cgiCookie);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiCookieInteger(
	char *name, int *result, int defaultV,char * cgiCookie);

cgiFormResultType cgiCookies(
	char ***ptrToStringArray,char* cgiCookie);

/* path can be null or empty in which case a path of / (entire site) is set. 
	domain can be a single web site; if it is an entire domain, such as
	'boutell.com', it should begin with a dot: '.boutell.com' */
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderCookieSetString(char *name, char *value, 
	int secondsToLive, char *path, char *domain,FCGX_Stream *out);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderCookieSetInteger(char *name, int value,
	int secondsToLive, char *path, char *domain,FCGX_Stream *out);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderLocation(char *redirectUrl,FCGX_Stream * out);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderStatus(int status, char *statusMessage,FCGX_Stream * out);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderContentType(char *mimeType,FCGX_Stream * out);

typedef enum {
	cgiEnvironmentIO,
	cgiEnvironmentMemory,
	cgiEnvironmentSuccess,
	cgiEnvironmentWrongVersion
} cgiEnvironmentResultType;

extern 
#ifdef __cplusplus
"C" 
#endif
cgiEnvironmentResultType cgiWriteEnvironment(char *filename);
extern cgiEnvironmentResultType cgiReadEnvironment(char *filename);

extern 
#ifdef __cplusplus
"C" 
#endif
int cgiMain();
extern 


#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormEntries(
	char ***ptrToStringArray,struct cgi_env **ce);

/* Output string with the <, &, and > characters HTML-escaped. 
	's' is null-terminated. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiHtmlEscape(char *s,FCGX_Stream *out);

/* Output data with the <, &, and > characters HTML-escaped. 
	'data' is not null-terminated; 'len' is the number of
	bytes in 'data'. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiHtmlEscapeData(char *data, int len,FCGX_Stream *out);

/* Output string with the " character HTML-escaped, and no
	other characters escaped. This is useful when outputting
	the contents of a tag attribute such as 'href' or 'src'.
	's' is null-terminated. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiValueEscape(char *s,FCGX_Stream *out);

/* Output data with the " character HTML-escaped, and no
	other characters escaped. This is useful when outputting
	the contents of a tag attribute such as 'href' or 'src'.
	'data' is not null-terminated; 'len' is the number of
	bytes in 'data'. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiValueEscapeData(char *data, int len,FCGX_Stream *out);

int cgiMain_init(int argc, char *argv[],struct cgi_env ** c,FCGX_Request *);

void cgiFreeResources(struct cgi_env ** c);

#endif /* CGI_C */

