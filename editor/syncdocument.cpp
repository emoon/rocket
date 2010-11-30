#include "syncdocument.h"
#include <string>

SyncDocument::~SyncDocument()
{
	clearUndoStack();
	clearRedoStack();
}

#import <msxml3.dll> named_guids

bool SyncDocument::load(const std::wstring &fileName)
{
	MSXML2::IXMLDOMDocumentPtr doc(MSXML2::CLSID_DOMDocument);
	try
	{
		SyncDocument::MultiCommand *multiCmd = new SyncDocument::MultiCommand();
		
		doc->load(fileName.c_str());
		MSXML2::IXMLDOMNamedNodeMapPtr attribs = doc->documentElement->Getattributes();
		MSXML2::IXMLDOMNodePtr rowsParam = attribs->getNamedItem("rows");
		if (NULL != rowsParam)
		{
			std::string rowsString = rowsParam->Gettext();
			this->setRows(atoi(rowsString.c_str()));
		}
		
		MSXML2::IXMLDOMNodeListPtr trackNodes = doc->documentElement->selectNodes("track");
		for (int i = 0; i < trackNodes->Getlength(); ++i)
		{
			MSXML2::IXMLDOMNodePtr trackNode = trackNodes->Getitem(i);
			MSXML2::IXMLDOMNamedNodeMapPtr attribs = trackNode->Getattributes();
			
			std::string name = attribs->getNamedItem("name")->Gettext();

			// look up track-name, create it if it doesn't exist
			size_t trackIndex;
			for (trackIndex = 0; trackIndex < tracks.size(); ++trackIndex)
				if (name == tracks[trackIndex]->name)
					break;

			if (trackIndex == tracks.size())
				trackIndex = int(createTrack(name));

			MSXML2::IXMLDOMNodeListPtr rowNodes = trackNode->GetchildNodes();
			for (int i = 0; i < rowNodes->Getlength(); ++i)
			{
				MSXML2::IXMLDOMNodePtr keyNode = rowNodes->Getitem(i);
				std::string baseName = keyNode->GetbaseName();
				if (baseName == "key")
				{
					MSXML2::IXMLDOMNamedNodeMapPtr rowAttribs = keyNode->Getattributes();
					std::string rowString = rowAttribs->getNamedItem("row")->Gettext();
					std::string valueString = rowAttribs->getNamedItem("value")->Gettext();
					std::string interpolationString = rowAttribs->getNamedItem("interpolation")->Gettext();

					KeyFrame k;
					int row = atoi(rowString.c_str());
					k.value = float(atof(valueString.c_str()));
					k.type = KeyFrame::Type(atoi(interpolationString.c_str()));
					multiCmd->addCommand(new InsertCommand(int(trackIndex), row, k));
				}
			}
		}
		this->exec(multiCmd);
		savePointDelta = 0;
		savePointUnreachable = false;
	}
	catch(_com_error &e)
	{
		char temp[256];
		_snprintf(temp, 256, "Error loading: %s\n", (const char*)_bstr_t(e.Description()));
		MessageBox(NULL, temp, NULL, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
		return false;
	}
	
	return true;
}

bool SyncDocument::save(const std::wstring &fileName)
{
	MSXML2::IXMLDOMDocumentPtr doc(MSXML2::CLSID_DOMDocument);
	try
	{
		char temp[256];
		_variant_t varNodeType((short)MSXML2::NODE_ELEMENT);
		MSXML2::IXMLDOMElementPtr rootNode = doc->createElement("tracks");
		doc->appendChild(rootNode);

		_snprintf(temp, 256, "%d", getRows());
		rootNode->setAttribute("rows", temp);

		for (size_t i = 0; i < tracks.size(); ++i) {
			const SyncTrack *t = tracks[i];

			MSXML2::IXMLDOMElementPtr trackElem = doc->createElement("track");
			trackElem->setAttribute("name", t->name.c_str());

			rootNode->appendChild(doc->createTextNode("\n\t"));
			rootNode->appendChild(trackElem);

			std::map<int, KeyFrame>::const_iterator it;
			for (it = t->keys.begin(); it != t->keys.end(); ++it) {
				size_t row = it->first;
				float value = it->second.value;
				char interpolationType = char(it->second.type);

				MSXML2::IXMLDOMElementPtr keyElem = doc->createElement("key");
				
				_snprintf(temp, 256, "%d", row);
				keyElem->setAttribute("row", temp);

				_snprintf(temp, 256, "%f", value);
				keyElem->setAttribute("value", temp);

				_snprintf(temp, 256, "%d", interpolationType);
				keyElem->setAttribute("interpolation", temp);

				trackElem->appendChild(doc->createTextNode("\n\t\t"));
				trackElem->appendChild(keyElem);
			}
			if (t->keys.size())
				trackElem->appendChild(doc->createTextNode("\n\t"));
		}
		if (tracks.size())
			rootNode->appendChild(doc->createTextNode("\n"));
		
		doc->save(fileName.c_str());
		
		savePointDelta = 0;
		savePointUnreachable = false;
	}
	catch(_com_error &e)
	{
		char temp[256];
		_snprintf(temp, 256, "Error saving: %s\n", (const char*)_bstr_t(e.Description()));
		MessageBox(NULL, temp, NULL, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
		return false;
	}
	return true;
}

