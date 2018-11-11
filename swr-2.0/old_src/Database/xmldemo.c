#include <stdio.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

typedef enum { FALSE = 0, TRUE } bool;

extern int errno;

bool file_exist(const char* filename);

void create_db(const char* filename);
void load_db(const char* filename);
void init_db(const char* filename);
void save_db(const char* filename);

/**
 * Checks if the file at the supplied file name exist.
 *
 * @param filename The filename of the file to check.
 * @returns True if it exist and false if it does not.
 */
bool file_exist(const char* filename)
{
	struct stat buf;

	if (stat(filename, &buf) == 0)
		return TRUE;
	else
		return FALSE;
}

/**
 * Creates a new database file and sets the initial contents.
 *
 * @param filename The filename of the file that the new database should be created using.
 */
void create_db(const char* filename)
{
	xmlDocPtr doc;
	xmlNodePtr root_node, node;

	doc = xmlNewDoc(BAD_CAST "1.0");

	root_node = xmlNewNode(NULL, BAD_CAST "Players");
	xmlDocSetRootElement(doc, root_node);

	node = xmlNewNode(NULL, BAD_CAST "Player");
	xmlNewChild(node, NULL, BAD_CAST "Stat1", BAD_CAST "StatValue1");
	xmlNewChild(node, NULL, BAD_CAST "Stat2", BAD_CAST "StatValue2");
	xmlNewChild(node, NULL, BAD_CAST "Stat3", BAD_CAST "StatValue3");
	xmlNewProp(node, BAD_CAST "ID", BAD_CAST "SomeAttrValue");
	xmlAddChild(root_node, node);

	xmlSaveFormatFile(filename, doc, 1);

	xmlFreeDoc(doc);
}

/**
 * Loads the data from the specified file to memory. If the file does not exist then
 * the file is created and initialized.
 * 
 * @param filename The filename of the file that that database should be loaded from.
 */
void load_db(const char* filename)
{
	xmlDocPtr doc;
	xmlNodePtr root_node, node;
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;
	xmlNodeSetPtr nodes;
	int size = 0;

	if (file_exist(filename) == TRUE)
	{
		if ((doc = xmlParseFile(filename)) == NULL)
		{
			/* fatal error */
			return;
		}
		
		if ((context = xmlXPathNewContext(doc)) == NULL)
		{
			/* fatal error */
			return;
		}

		if ((object = xmlXPathEvalExpression("..\\Player", context) == NULL)
		{
			/* fatal error */
			return;
		}

		object->nodesetval

		

		xmlXPathFreeObject(object);
		xmlXPathFreeContext(context);
		xmlFreeDoc(doc);
	}
	else
	{
		return;
	};
}

int main()
{
	xmlInitParser();
	LIBXML_TEST_VERSION

	create_db("test2.xml");
	load_db("test2.xml");

	xmlCleanupParser();
}
