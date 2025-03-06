Here's the documentation for the classes in the namemaps.hh file:

### Name_Maps Class
**Purpose:**  
Maintains comprehensive mappings of all names and their relationships within a program. This class serves as a central repository for class, method, and variable declarations, facilitating easy access during compilation.

**Key Components:**
- **classes:** Set of all class names
- **classHierachy:** Map of class inheritance relationships
- **methods:** Set of class-method name pairs
- **classVar:** Map of class variables to their declarations
- **methodVar:** Map of method variables to their declarations
- **methodFormal:** Map of method formal parameters to their declarations
- **methodFormalList:** Map of method formal parameter lists

**Key Methods:**
- **Class Management:**
  - is_class(): Checks if a class exists
  - add_class(): Adds a new class
  - add_class_hiearchy(): Adds class inheritance relationship
  - get_ancestors(): Retrieves ancestor classes

- **Method Management:**
  - is_method(): Checks if a method exists
  - add_method(): Adds a new method

- **Variable Management:**
  - is_class_var(): Checks if a class variable exists
  - add_class_var(): Adds a class variable
  - get_class_var(): Retrieves class variable declaration
  - is_method_var(): Checks if a method variable exists
  - add_method_var(): Adds a method variable
  - get_method_var(): Retrieves method variable declaration

- **Formal Parameter Management:**
  - is_method_formal(): Checks if a formal parameter exists
  - add_method_formal(): Adds a formal parameter
  - get_method_formal(): Retrieves formal parameter declaration
  - add_method_formal_list(): Adds formal parameter list
  - get_method_formal_list(): Retrieves formal parameter list

- **Utility:**
  - print(): Prints all name maps

### AST_Name_Map_Visitor Class
**Purpose:**  
Implements the visitor pattern for building name maps by traversing the AST. This class collects and organizes all name-related information from the program's AST.

**Key Components:**
- **name_maps:** Reference to the Name_Maps instance being built
- **current_visiting_class:** Tracks the current class being visited
- **current_visiting_method:** Tracks the current method being visited

**Key Methods:**
- **Constructor:** Initializes the visitor with a new Name_Maps instance
- **getNameMaps:** Returns the constructed Name_Maps instance
- **visit methods:** Overrides for each AST node type to collect name information

### Utility Function
- **makeNameMaps:** Creates and returns a Name_Maps instance for a given program
