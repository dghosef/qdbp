type loc = Lexing.position

type argument = {name: string; value: expr}
and record_field = 
| Field of {self: string; field_name: string; field_value: expr}
| Destructure of {e: expr}
and variant_message = {tag: string; fn: expr}
and expr = 
    | Record          of {location: loc; fields: record_field list}
    | Variant         of {location: loc; tag: string; value: expr}
    | Closure         of {location: loc; args: string list; body: expr}
    | Record_Message  of {location: loc; receiver: expr; message: string; arguments: argument list}
    | Variant_Message of {location: loc; receiver: expr; message: variant_message list}
    | Sequence        of {location: loc; l: expr; r: expr}
    | Assignment      of {location: loc; lhs: string; rhs: expr}
    | Variable        of {location: loc; name: string}
    | Int             of {location: loc; value: int}

let emit_record_message_arg n v = {name = n; value = v}
let emit_variant_message t f = {tag = t; fn = f}