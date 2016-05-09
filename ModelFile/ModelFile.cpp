#include "ModelFile.hpp"

#include <map>
#include <vector>

using namespace std;
using namespace epanet;

#define ENDL '\n'

void _write_title(OW_Project* m, ostream& s);
void _read_title(OW_Project* m, istream& s);

void _write_junctions(OW_Project* m, ostream& s);

void _read_blank(OW_Project* m, istream& s);
void _write_blank(OW_Project* m, ostream& s);


namespace epanet {
  class InpSection {
  public:
    InpSection(string section_name, int section_type, function<void(OW_Project*,ostream&)> serializer_function, function<void(OW_Project*,istream&)> deserializer_function)
    : sectionStr(section_name), sectionType(section_type), serializer(serializer_function), deserializer(deserializer_function) { };
    string sectionStr;
    int sectionType;
    function<void(OW_Project*,ostream&)> serializer;
    function<void(OW_Project*,istream&)> deserializer;
  };
}

enum _inp_section_t : int {
  TITLE = 0,
  JUNCTIONS, RESERVOIRS, TANKS,
  PIPES, PUMPS, VALVES,
  CONTROLS, RULES,
  DEMANDS, SOURCES, EMITTERS, PATTERNS, CURVES,
  QUALITY, STATUS, ROUGHNESS, ENERGY, REACTIONS, MIXING,
  REPORT, TIMES, OPTIONS,
  COORDS, VERTICES,
  LABELS, BACKDROP, TAGS, END
};

vector<InpSection> _sections = {
  
  InpSection("TITLE",     TITLE,     _write_title,     _read_title),
  InpSection("JUNCTIONS", JUNCTIONS, _write_junctions, _read_blank)
  
};


ModelFile::ModelFile() : _model(NULL) {
  
}

ModelFile::ModelFile(OW_Project *model) : _model(model) {
  
};

ModelFile::ModelFile(istream& input) {
  this->_newModelFromStream(input);
}

OW_Project* ModelFile::to_model() {
  return _model;
}

void ModelFile::_newModelFromStream(istream &stream) {
  
  int err;
  err = OW_newModel(&_model);
  
}

void ModelFile::to_stream(ostream &stream) {
  
  if (_model == NULL) {
    return;
  }
  
  stream << "[TITLE]" << ENDL;
  
}






