#include <Windows.h>
#include <string>
#include <sstream>
#define CURL_STATICLIB
#include <curl\curl.h>
#pragma comment(lib,"libcurl")
#pragma comment(lib,"Ws2_32")
#include <json/json.h>
#pragma comment(lib,"lib_json")


// pushbullet.cpp : Defines the exported functions for the DLL application.
//

class pushbullet
{
public:
	pushbullet();
	~pushbullet();
	short note(const std::string token_key, const std::string    title, const std::string    body);
	short file(const std::string token_key, const std::string    title, const std::string    body, const std::string    path);

private:
	short post_request(const std::string token_key, const std::string request_url, std::string *result, const std::string data);
	short upload_request(const std::string token_key, const std::string  path, Json::Value        *json_request);
	short form_request(const std::string    url_request, const Json::Value    data, const std::string    path, std::string          *result);
	static size_t WriteMemoryCallback(void      *contents,
		size_t    size,
		size_t    nmemb,
		void      *userp
	)
	{
		((std::string *) userp)->append((char *)contents, size * nmemb);

		return (size * nmemb);
	}
};


pushbullet::pushbullet()
{
}

pushbullet::~pushbullet()
{
}

short pushbullet::form_request(const std::string    url_request,
	const Json::Value    data,
	const std::string    path,
	std::string          *result
)

{

	/*  Start a libcurl easy session
	*/
	CURL            *s;
	CURLcode        r = CURLE_OK;

	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	curl_slist      *http_headers = NULL;


	curl_global_init(CURL_GLOBAL_ALL);

	/* Fill in all form datas
	*/
	for (Json::Value::const_iterator itr = data.begin(); itr != data.end(); itr++)
	{
		std::stringstream     tmp;

		tmp << itr.key().asString();
		curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, tmp.str().c_str(),
			CURLFORM_COPYCONTENTS, std::string(itr->asString()).c_str(),
			CURLFORM_END);
	}

	/* Fill in the file upload field
	*/
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "file",
		CURLFORM_FILE, path.c_str(),
		CURLFORM_END);


	/* Initialize the session
	*/
	s = curl_easy_init();
	if (s)
	{
		http_headers = curl_slist_append(http_headers, "Content-Type: multipart/form-data");
		curl_easy_setopt(s, CURLOPT_URL, url_request.c_str());
		curl_easy_setopt(s, CURLOPT_HTTPPOST, formpost);
		curl_easy_setopt(s, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(s, CURLOPT_WRITEDATA, &(*result));


		/* Get data
		*/
		r = curl_easy_perform(s);


		/* Checking errors
		*/
		if (r != CURLE_OK)
		{
			MessageBox(0, curl_easy_strerror(r), "curl_easy_perform() failed!", 0);
			return (-2);
		}

		curl_easy_cleanup(s);
		curl_formfree(formpost);
		curl_slist_free_all(http_headers);
	}
	else
	{
		MessageBox(0, "curl_easy_init() could not be initiated.", 0, 0);
		return (-1);
	}

	return (0);
}

short pushbullet::upload_request(const std::string token_key, const std::string  path, Json::Value        *json_request)
{

	FILE                    *fp;
	std::stringstream       data;
	std::string             file_name, file_type, result; // file_type corresponds to MIME type



	fopen_s(&fp, path.c_str(), "rb"); /* open file to upload */

	if (fp == NULL)
	{
		MessageBox(0, "screen.jpg not opened!", 0, 0);
		return (-2);
	}
	fclose(fp);


	file_name = "screen.jpg";//basename(strdup(path.c_str())); //_splitpath_s
	file_type = "image/jpg";
	data.str(std::string());
	data << "{"
		<< "\"file_name\" : \"" << file_name << "\", "
		<< "\"file_type\" : \"" << file_type << "\""
		<< "}";

	if (this->post_request(token_key, "https://api.pushbullet.com/v2/upload-request", &result, data.str()) != 0)
	{

		return (-1);
	}


	/* Need to return the dictionary
	*/
	std::stringstream(result) >> *json_request;


	return (0);
}

short pushbullet::file(const std::string token_key, const std::string    title,
	const std::string    body,
	const std::string    path
)
{
	Json::Value     json_request;
	std::string     result;
	std::stringstream     data;

	if (this->upload_request(token_key, path, &json_request) < 0)
	{
		return (-1);
	}


	/* Get the dictionary 'data'
	*/
	const Json::Value     data_json = json_request["data"];

	if (this->form_request(json_request["upload_url"].asString(), data_json, path, &result) < 0)
	{

		return (-1);
	}


	data << "{"
		<< "\"type\":\"file\", "
		<< "\"file_name\":\"" << json_request["file_name"].asString() << "\", "
		<< "\"file_type\":\"" << json_request["file_type"].asString() << "\", "
		<< "\"title\":\"" << title << "\", "
		<< "\"body\":\"" << body << "\", "
		<< "\"file_url\":\"" << json_request["file_url"].asString() << "\""
		<< "}";


	if (this->post_request(token_key, "https://api.pushbullet.com/v2/pushes", &result, data.str()) != 0)
	{
		return (-1);
	}

	return (0);
}
short pushbullet::post_request(const std::string token_key, const std::string request_url, std::string *result, const std::string data)
{

	/*  Start a libcurl easy session
	*/
	CURL     *s = curl_easy_init();

	result->clear();

	if (s)
	{
		CURLcode        r = CURLE_OK;
		curl_slist      *http_headers = NULL;

		http_headers = curl_slist_append(http_headers, "Content-Type: application/json");


		/*  Specify URL to get
		*  Specify the user using the token key
		*  Specify that we are going to post
		*  Specify the data we are about to send
		*  Specify the HTTP header
		*  Send all data to the WriteMemoryCallback method
		*/
		curl_easy_setopt(s, CURLOPT_URL, request_url.c_str());
		curl_easy_setopt(s, CURLOPT_USERPWD, token_key.c_str());
		curl_easy_setopt(s, CURLOPT_POSTFIELDS, data.c_str());
		curl_easy_setopt(s, CURLOPT_HTTPHEADER, http_headers);
		curl_easy_setopt(s, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(s, CURLOPT_WRITEDATA, &(*result));

		/* Get data
		*/
		r = curl_easy_perform(s);

		/* Checking errors
		*/
		if (r != CURLE_OK)
		{
			MessageBox(0, curl_easy_strerror(r), "curl_easy_perform() failed!",0);
			return (-2);
		}

		curl_easy_cleanup(s);
		curl_slist_free_all(http_headers);
	}
	else
	{
		MessageBox(0, "curl_easy_init() could not be initiated.",0, 0);
		return (-1);
	}

	return (0);
}
short pushbullet::note(const std::string token_key, const std::string    title, const std::string    body)
{
	std::stringstream       data;
	std::string             result;


	data << "{"
		<< "\"type\":\"note\" , "
		<< "\"title\":\"" << title << "\" , "
		<< "\"body\":\"" << body << "\""
		<< "}";

	if (this->post_request(token_key, "https://api.pushbullet.com/v2/pushes", &result, data.str()) != 0)
	{
		return (-1);
	}

	return (0);
}











// dllmain.cpp : Defines the entry point for the DLL application.

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}



#include <adk/Plugin.h>
#include <adk/Plugin_Legacy.h>
// These are the only two lines you need to change
#define PLUGIN_NAME "Pushbullet Notifications Sender Plug-in"
#define VENDOR_NAME "https://github.com/mumin16/"
#define PLUGIN_VERSION 10000

////////////////////////////////////////
// Data section
////////////////////////////////////////
static struct PluginInfo oPluginInfo =
{
	sizeof(struct PluginInfo),
	1,
	PLUGIN_VERSION,
	0,
	PLUGIN_NAME,
	VENDOR_NAME,
	0,
	371000
};

// the site interface for callbacks
struct SiteInterface gSite;

///////////////////////////////////////////////////////////
// Basic plug-in interface functions exported by DLL
///////////////////////////////////////////////////////////

PLUGINAPI int GetPluginInfo(struct PluginInfo *pInfo)
{
	*pInfo = oPluginInfo;

	return TRUE;
}


PLUGINAPI int SetSiteInterface(struct SiteInterface *pInterface)
{
	gSite = *pInterface;

	return TRUE;
}


PLUGINAPI int GetFunctionTable(FunctionTag **ppFunctionTable)
{
	*ppFunctionTable = gFunctionTable;

	// must return the number of functions in the table
	return gFunctionTableSize;
}

PLUGINAPI int Init(void)
{
	return 1; 	 // default implementation does nothing

};

PLUGINAPI int Release(void)
{
	return 1; 	  // default implementation does nothing
};



AmiVar VSendNote(int NumArgs, AmiVar *ArgsTable)
{

	AmiVar result;

	result = gSite.AllocArrayResult();

	pushbullet* push=new pushbullet();
	push->note(ArgsTable[0].string, ArgsTable[1].string, ArgsTable[2].string);
	delete push;
	
	
	result.string = "sendnote success";
	return result;
}

AmiVar VSendScreenCapture(int NumArgs, AmiVar *ArgsTable)
{

	AmiVar result;

	result = gSite.AllocArrayResult();



	pushbullet* push = new pushbullet();
	char curdir[1024];
	GetCurrentDirectory(1024, curdir);
	std::string path = curdir;
	path.append("\\screen.jpg");
	push->file(ArgsTable[0].string, ArgsTable[1].string, ArgsTable[2].string,path);
	delete push;


	result.string = "sendscreencapture success";
	return result;
}

/////////////////////////////////////////////
// Function table now follows
//
// You have to specify each function that should be
// visible for AmiBroker.
// Each entry of the table must contain:
// "Function name", { FunctionPtr, <no. of array args>, <no. of string args>, <no. of float args>, <no. of default args>, <pointer to default values table float *>

FunctionTag gFunctionTable[] = {
	"SendNote",{ VSendNote, 0,3, 0, 0, NULL },
	"SendScreenCapture",{ VSendScreenCapture, 0,3, 0, 0, NULL }
	
	
};

int gFunctionTableSize = sizeof(gFunctionTable) / sizeof(FunctionTag);
