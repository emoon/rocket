#include "syncdocument.h"
#include <string>

SyncDocument::~SyncDocument()
{
	clearUndoStack();
	clearRedoStack();
}

void SyncDocument::sendSetKeyCommand(uint32_t track, int row)
{
	if (!clientSocket.connected())
		return;
	if (!clientRemap.count(track))
		return;

	SyncTrack *t = tracks[track];
	track = htonl(clientRemap[track]);

	std::map<int, KeyFrame>::const_iterator it = t->keys.find(row);
	assert(it != t->keys.end());

	for (int i = 0; i < 2; ++i) {
		union {
			float f;
			uint32_t i;
		} v[4];

		v[0].f = it->second.value;

		switch (it->second.type) {
		case KeyFrame::TYPE_STEP:
			v[1].f = 0.0f;
			v[2].f = 0.0f;
			v[3].f = 0.0f;
			break;
		case KeyFrame::TYPE_LINEAR:
			v[1].f = 1.0f;
			v[2].f = 0.0f;
			v[3].f = 0.0f;
			break;
		case KeyFrame::TYPE_SMOOTH:
			v[1].f =  0.0f;
			v[2].f =  3.0f;
			v[3].f = -2.0f;
			break;
		case KeyFrame::TYPE_RAMP:
			v[1].f = 0.0f;
			v[2].f = 1.0f;
			v[3].f = 0.0f;
			break;
		default:
			assert(false);
		}

		for (int j = 0; j < 4; ++j)
			v[j].i = htonl(v[j].i);

		assert(it->second.type < KeyFrame::TYPE_COUNT);

		unsigned char cmd = SET_KEY;
		uint32_t nrow = htonl((uint32_t)it->first);

		clientSocket.send((char *)&cmd, 1, 0);
		clientSocket.send((char *)&track, sizeof(track), 0);
		clientSocket.send((char *)&nrow, sizeof(nrow), 0);
		clientSocket.send((char *)&v[0].i, sizeof(v[0].i), 0);
		clientSocket.send((char *)&v[1].i, sizeof(v[1].i), 0);
		clientSocket.send((char *)&v[2].i, sizeof(v[2].i), 0);
		clientSocket.send((char *)&v[3].i, sizeof(v[3].i), 0);

		if (--it == t->keys.end())
			break;
	}
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

