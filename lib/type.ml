type basictype =
    | IntType
type type_name_map = (string, datatype) Hashtbl.t
and type_var = {id: int}
and algebraic_datatype = {fields: type_name_map; extension: datatype option}
and forall_type = {var: type_var; tp: datatype}
and datatype =
    | TypeVar       of type_var
    | BasicType     of basictype
    | RecordType    of algebraic_datatype
    | RecordTypeExt of algebraic_datatype
    | VariantType   of algebraic_datatype
    | UBVariant     of algebraic_datatype
    | LBVariant     of algebraic_datatype
    | ForAllType    of forall_type