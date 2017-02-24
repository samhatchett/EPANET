#ifndef ModelFile_hpp
#define ModelFile_hpp

#include <stdio.h>
#include <iostream>
#include <string>

#include "epanet2.h"

namespace epanet {
  class ModelFile {
  public:
    ModelFile(); ///! empty constructor
    ModelFile(std::istream& input); ///! instantiate with text file
    ModelFile(EN_Project *model); ///! instantiate with model project
    
    void to_stream(std::ostream& stream); ///! export model as text file, also can use << operator
    EN_Project* to_model(); ///! retrieve model representation of file
    
  private:
    EN_Project *_model;
    void _newModelFromStream(std::istream &stream);
  };
  std::ostream& operator<<(std::ostream& out, const ModelFile& modelFile); // stream out of model file
  std::istream& operator>>(std::istream& in, ModelFile& modelFile); // stream into model file
}


#endif /* ModelFile_hpp */
