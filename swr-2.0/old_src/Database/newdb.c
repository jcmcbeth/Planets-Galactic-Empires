/**
 * Contains the functions dealing with reading/writing using the new XML
 * database.
 *
 * These functions mainly use the XML tree library which tries to follow the
 * XML DOM. Although, this requires the entire document to be loaded into
 * memory and then some. So, if the XML files grow too large this might have to
 * be revised to use xmlreader/xmlwriter libraries instead which are not as
 * simple to use.
 *
 * @author	Joel McBeth
 * @version	1.0, 01/15/05
 */

#include <sys/stat.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "mud.h"


/**
 * Checks if the file at the supplied file name exist.
 *
 * @param filename The filename of the file to check.
 * @returns True if it exist and false if it does not.
 */
bool file_exist(const char* filename)
{
	struct stat buf;

	/* If the stat completed successfully then the file exist. */	 
	if (stat(filename, &buf) == 0)
		return TRUE;
	else
		return FALSE;
}

/**
 * Evaluates an XPath expression.
 *
 * @param doc The document to use to evaluate the expression.
 * @param expr The expression to evaluate.
 *
 * @returns A set of nodes that matched the expression, NULL if none were found or it failed.
 */
xmlNodeSet* xpath_expr(xmlDocPtr doc, const xmlChar* expr)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;	

	if ((context = xmlXPathNewContext(doc)) == NULL)
	{		
		return NULL;
	}

	/* This is where is evaluates the expression */
	if ((object = xmlXPathEvalExpression(expr, context)) == NULL)
	{
		xmlXPathFreeContext(context);		

		return NULL:
	}

	xmlXPathFreeContext(context);	

	return object;
}

/**
 * Saves the contents of a <code>SYSTEM_DATA</code> structure to file in XML.
 *
 * @param data The data to save to file.
 * @param filename The the name of the file that will be used to save the data. 
 *
 * @returns If the data was successfully saved to file or not.
 */
bool db_save_systemdata(SYSTEM_DATA* data, const char* filename)
{	
	xmlDocPtr doc;
	xmlNodePtr root_node, node;	

	sprintf(filename, "%sConfig.xml", SYSTEM_DIR);

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, "Config");

	return TRUE;
}

/**
 * Allocates memory and initializes values.
 *
 * @returns An allocated and initialized <code>SYSTEM_DATA</code> structure.
 */
SYSTEM_DATA* new_systemdata()
{
	SYSTEM_DATA *data;

	/* Allocate the memory for the structure. */
	CREATE(data, SYSTEM_DATA, 1);

	/* Initialize the values and allocate memory for pointers. */
	data->maxplayers = 0;
	data->alltimemax = 0;	
	data->time_of_max = str_dup(""); /* Doesn't need to be hashed. */
	data->offcials = str_dup(""); /* Doesn't need to be hashed. */
	data->multiplayers = str_dup(""); /* Doesn't need to be hashed. */
	data->NO_NAME_RESOLVING = FALSE;
	data->DENY_NEW_PLAYERS = FALSE:
	data->WAIT_FOR_AUTH = FALSE;
	data->max_sn = 0;
	data->save_flags = 0;
	data->save_frequency = 0;

	return data;
}

/**
 * Frees all the memory associated with a <code>SYSTEM_DATA</code> structure.
 *
 * @param data The data to be freed from memory.
 */
void free_systemdata(SYSTEM_DATA *data)
{
	if (data != NULL)
	{
		/* Free the non-hashed strings. */
		DISPOSE(data->time_of_max);
		DISPOSE(data->officials);
		DISPOSE(data->multiplayers);

		/* Free the rest. */
		DISPOSE(data);
	}
}

/**
 * Loads system data from an XML file.
 *
 * @param data The data read from the file will be loaded into this.
 * @param data The filename of the file that the data will be loaded from.
 *
 * @returns If loading the data was successful or not.
 */
bool db_load_systemdata(SYSTEM_DATA* data, const char* filename)
{
	xmlDoc* doc;	
	xmlXPathObject* object;
	int size, i;

	/* If the file does not exist then it can't be loaded. */	
	if (file_exist(filename) == FALSE)
	{
		bug("db_load_systemdata: There was no config file.");

		return FALSE;
	}

	/* Load the document from the supplied filename. */
	if ((doc = xmlParseFile(filename)) == NULL)
	{
		xmlFreeDoc(doc);

		return FALSE;
	}

	/* TODO: Add schema validation. */

	/* Get the max players. */
	if ((object = xpath_expr(doc, BAD_CAST "Config//MaxPlayers")) != NULL)		
	{
		data->maxplayers = atoi(xmlNodeGetContent(object->nodeTab[0]));
		xmlXPathFreeObject(object);
	}

	/* Get the all time max. */
	if ((object = xpath_expr(doc, BAD_CAST "Config//MaxPlayersTime")) != NULL)	
	{
		data->alltimemax = atoi(xmlNodeGetContent(object->nodeTab[0]));
		xmlXPathFreeObject(object);
	}

	/* Get the time of the all time max. */
	if ((object = xpath_expr(doc, BAD_CAST "Config//ResolveNames")) != NULL)	
	{
		data->NO_NAME_RESOLVING = atoi(xmlNodeGetContent(object->nodeTab[0]));
		xmlXPathFreeObject(object);
	}

	/* Get if new characters need to wait for authorization. */
	if ((object = xpath_expr(doc, BAD_CAST "Config//WaitForAuth")) != NULL)	
	{
		data->WAIT_FOR_AUTH = atoi(xmlNodeGetContent(object->nodeTab[0]));
		xmlXPathFreeObject(object);
	}

	/* Get if new characters need to wait for authorization. */
	if ((object = xpath_expr(doc, BAD_CAST "Config//SaveFlags")) != NULL)	
	{
		data->save_flags = atoi(xmlNodeGetContent(object->nodeTab[0]));
		xmlXPathFreeObject(object);
	}
	
	xmlFreeDoc(doc);

	return TRUE;
}