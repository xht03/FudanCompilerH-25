This is the description of the classes in the semant.hh file.

### AST_Semant Class
**Purpose:**  
Represents semantic information for an AST (Abstract Syntax Tree) node, primarily used for expressions and variables. This information is crucial for type checking and IR (Intermediate Representation) generation.

**Key Components:**
- **Kind Enum:** Defines the type of semantic entity (Value, MethodName, ClassName)
- **TypeKind:** Stores the type information (CLASS/OBJECT, INT, ARRAY)
- **type_par:** Variant that can hold additional type parameters (class name or array arity)
- **lvalue:** Indicates whether the expression is an lvalue

**Methods:**
- Constructor: Initializes all semantic attributes
- get_kind(): Returns the kind of semantic entity
- get_type(): Returns the type kind
- get_type_par(): Returns type parameters
- is_lvalue(): Checks if the expression is an lvalue
- s_kind_string(): Converts Kind enum to string representation

### AST_Semant_Map Class
**Purpose:**  
Maps AST nodes to their corresponding semantic information (AST_Semant objects). This mapping is essential for maintaining semantic context throughout the compilation process.

**Key Components:**
- **semant_map:** Map structure storing AST node to AST_Semant associations
- **name_maps:** Reference to name mapping information

**Methods:**
- Constructor/Destructor: Manages the map lifecycle
- getSemant(): Retrieves semantic information for a given AST node
- setSemant(): Associates semantic information with an AST node

### AST_Semant_Visitor Class
**Purpose:**  
Implements the visitor pattern for semantic analysis of the AST. It traverses the AST, performs semantic checks, and maintains semantic information for all subexpressions.

**Key Components:**
- **semant_map:** Stores semantic information for all subexpressions
- **name_maps:** Map of all names in the program
- **current_visiting_class/method:** Tracks current context
- **deferred_id:** Set of identifier expressions requiring deferred resolution
- **in_a_while_loop:** Counter for handling nested while loops

**Methods:**
- Constructor: Initializes with name maps
- getSemantMap(): Returns the semantic map
- visit() methods: Overrides for each AST node type to perform specific semantic analysis

### Utility Functions
- **makeNameMaps:** Creates name mappings for a program
- **semant_analyze:** Performs semantic analysis on a program

This documentation provides a clear overview of the classes and their purposes without modifying the actual code. Each class's key components and methods are described to help understand their roles in the semantic analysis process.