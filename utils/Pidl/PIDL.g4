/* 본 파일 작업 요령:
Visual Studio Addon 설치: ANTLR Tools, T4 template tools

참고:
본 파일을 수정하면 Debug or Release 하위 폴더에 parser,lexer 소스코드가 생성되며 자동 참조됩니다.
*/

grammar PIDL;

@parser::header {
using System;
using System.IO;
using System.Linq;
using System.Text;

}
@parser::members {
}
 
// Compilation Unit: In PIDL, this is a single file.  This is the start
//   rule for this parser
compilationUnit returns [ Parsed_Root ret ]
@init { $ret = new Parsed_Root(); }
	:	
		(
			a_import=importDef { $ret.m_imports.AddOnLangMatch($a_import.ret); }
			| a_include=includeDef { $ret.m_includes.AddOnLangMatch($a_include.ret); }
			| a_using=usingDef { $ret.m_usings.AddOnLangMatch($a_using.ret); }
			| a_package = packageDefinition { $ret.m_package = $a_package.ret; }
			| global_interface=globalInterfaceDef { $ret.m_globalInterfaces.Add($global_interface.ret); } 
			| aliasDef
			| c = componentDef { $ret.m_components.Add($c.ret); }
			| dllExportDef
			| m=moduleDef { $ret.m_module = $m.ret; }
			| sn = codeSnippetDef { $ret.m_codeSnippets.Add($sn.ret); }
		) *
		EOF
	;

dllExportDef
	:
		KW_DLLEXPORT def=IDENTIFIER { App.SetExportDef($def.text); }
		SEMICOLON
	;

moduleDef returns [ Parsed_Module ret ]
@init { $ret = new Parsed_Module(); }
	:
		KW_MODULE name=IDENTIFIER { $ret.m_name = $name.text; }
		SEMICOLON
	;

// 예시: component MyGame.Player {... }
componentDef returns [ Parsed_Component ret]
@init { $ret = new Parsed_Component(); }
	:		
		(a=attrListDef { $ret.m_attrList = $a.ret; })? // attr 지정은 사용자 선택사항이다.
		KW_COMPONENT c = variableType { $ret.m_className = $c.ret; }
		OPEN_BRACE 
			(f=componentFieldDef { $ret.m_fields.Add($f.ret); }
			| sn = codeSnippetDef { $ret.m_codeSnippets.Add($sn.ret); }
			)*
		CLOSE_BRACE
		SEMICOLON?
		{ $ret.CheckValidation(); }		
	;

codeSnippetDef returns [ string ret ]
	:
		OPEN_SNIPPET
		t=anyTerm { $ret=$t.text; }
		CLOSE_SNIPPET
	;

attrListDef returns [ Parsed_AttrList ret ]
@init { $ret = new Parsed_AttrList(); }
	:
		OPEN_BRACKET
		(a=attrDef { $ret.Add($a.ret); })*
		CLOSE_BRACKET
	;

// 예시: xxx(yyy) 혹은 xxx 혹은 xxx(123) 혹은 xxx("asdasd")
attrDef returns [ Parsed_Attr ret ]
@init { $ret = new Parsed_Attr(); }
	:
		// name
		i=IDENTIFIER { $ret.m_name = $i.text; }  
		// value
		(									   
			OPEN_PARENS 
			(
				v=IDENTIFIER {$ret.m_value = $v.text; } 
				| n=INTEGER_LITERAL {$ret.m_value = $n.text; } 
				| s=STRING_LITERAL {$ret.m_value = $s.text; }
			)
			CLOSE_PARENS 
		)?
	;

// 예시: map<string, int*> myField1 = GetSomething(); 
//       map<string, int*> myField1; 
componentFieldDef returns [ Parsed_Field ret ]
@init { $ret = new Parsed_Field(); }
	:
		(a=attrListDef { $ret.m_attrList = $a.ret; } ) ? 
		t=variableType n=IDENTIFIER { $ret.m_type = $t.ret; $ret.m_name = $n.text; }
		(ASSIGNMENT d=anyTerm { $ret.m_defaultValue = $d.text; } )?
		SEMICOLON
		
	;

anyTerm 
	:
		.*?
	;

aliasDef 
	:	KW_RENAME langName=IDENTIFIER 
		OPEN_PARENS name1=variableType COMMA name2=variableType CLOSE_PARENS SEMICOLON 
		{ App.AddAlias($langName.text,$name1.ret,$name2.ret); } 
	;

importDef returns [Parsed_Using ret]
@init { $ret = new Parsed_Using(); }
	:
		KW_IMPORT ( OPEN_PARENS langName=IDENTIFIER { $ret.m_langName = $langName.text; } CLOSE_PARENS )? vt=variableType SEMICOLON 
		{ 
			if($ret.m_langName == null) 
			{
				$ret.m_langName = "java"; 
				App.ParseWarn("import keyword requires language space (e.g. import(as) xxx.yyy; ). Java is assumed instead."); 
			} 
			$ret.m_name=$vt.ret; 
		}
	;

usingDef returns [Parsed_Using ret]
@init { $ret = new Parsed_Using(); }
	:
		KW_USING ( OPEN_PARENS langName=IDENTIFIER { $ret.m_langName = $langName.text; } CLOSE_PARENS )? vt=variableType SEMICOLON 
		{ 
			if($ret.m_langName == null)
			{
				$ret.m_langName = "cs";
				App.ParseWarn("using keyword requires language space (e.g. using(cs) xxx.yyy; ). CSharp is assumed instead."); 
			}
			$ret.m_name=$vt.ret; 
		}
	;

includeDef returns [Parsed_Using ret]
@init 
{
	$ret = new Parsed_Using(); 
	string vt = "";  // #include <> 과정에서 채워짐
}
	:
		SHARP KW_INCLUDE 
		(	
			fileName=STRING_LITERAL { $ret.m_name=$fileName.text; $ret.m_langName = "cpp"; } // #include ""
			|	LT fileName2=IDENTIFIER { vt += "<"; vt += $fileName2.text; } // #include <>
				(
					DOT fileName3=IDENTIFIER { vt += "."; vt += $fileName3.text; }
				)?
				GT { vt += ">"; $ret.m_name=vt; $ret.m_langName = "cpp"; }
		)
	;

globalInterfacesDef returns [ List<Parsed_GlobalInterface> ret ]
@init { $ret = new List<Parsed_GlobalInterface>(); }	
	:	( 
		global_interface=globalInterfaceDef { $ret.Add($global_interface.ret); }
		)*
	;
	
// RMI global interface 
globalInterfaceDef returns [ Parsed_GlobalInterface ret ]
@init { $ret = new Parsed_GlobalInterface(); }	
	:	(	OPEN_BRACKET gia=globalInterfaceAttr { $gia.ret.Apply($ret); }
			( COMMA gia2=globalInterfaceAttr { $gia2.ret.Apply($ret); } )* 
			CLOSE_BRACKET 
		)?
		KW_GLOBAL globalInterfaceName=variableType  { $ret.m_name=$globalInterfaceName.ret; }
		(
			firstMessageIDA=IDENTIFIER { $ret.m_firstMessageID=$firstMessageIDA.text; } | 
			( firstMessageIDB=INTEGER_LITERAL{ $ret.m_firstMessageID=$firstMessageIDB.text; } ) |
			( MINUS firstMessageIDC=INTEGER_LITERAL{ $ret.m_firstMessageID="-"+$firstMessageIDC.text; } ) // weird definition, I know. sorry.
		)
		OPEN_BRACE
		methods=methodsDef { $ret.m_methods=$methods.ret; } 
		CLOSE_BRACE
		{
			$ret.ChainElements();
		}
	;
	
globalInterfaceAttr returns [ Parsed_GlobalInterfaceAttr ret ]
@init { $ret = new Parsed_GlobalInterfaceAttr(); 
    var mar = new Parsed_Marshaler(); }
	:
		KW_MARSHALER ( OPEN_PARENS langName=IDENTIFIER { mar.m_langName = $langName.text; } CLOSE_PARENS )? ASSIGNMENT v=variableType 
		{ 
			mar.m_name = $v.ret; 

			if(mar.m_langName == null) 
			{
				mar.m_langName = "cs";
				App.ParseWarn("marshaler attribute requires language space (e.g. marshaler(cs)=xxx ). C# is assumed instead."); 
			}
			$ret.m_marshaler = mar; 
		}
		|
		KW_ACCESS ASSIGNMENT acc=modifier
		{
			$ret.m_accessibility = $acc.ret;
		}
	;
	
methodsDef returns [ List<Parsed_Method> ret ]
@init { $ret = new List<Parsed_Method>(); }
	:	( 
		method=methodDef { $ret.Add($method.ret); }
		)*
	;	
		
// RMI method 
methodDef returns [ Parsed_Method ret ]
@init { $ret = new Parsed_Method(); }	
	:	(mm = methodMode { $ret.m_mode=$mm.ret; }| ) // method mode is not mandatory.
		methodName=IDENTIFIER
		OPEN_PARENS
		pars=paramsDef
		CLOSE_PARENS
		SEMICOLON
		{
			$ret.m_name=$methodName.text;
			$ret.m_params=$pars.ret;
		}
	;
	
// RMI method mode
methodMode returns [ Parsed_MethodMode ret ]
@init { $ret = new Parsed_MethodMode(); }	
	:	OPEN_BRACKET
		name=IDENTIFIER ASSIGNMENT value_=messageIdDef { $ret.ProcessAttribute($name.text, $value_.ret); }
		CLOSE_BRACKET
	;	

messageIdDef returns [ string ret ]
	:
		i=IDENTIFIER { $ret = $i.text; }
		| ni=INTEGER_LITERAL { $ret = $ni.text; }
	;

// RMI param definition list 
paramsDef returns [ List<Parsed_Param> ret ]
@init { $ret = new List<Parsed_Param>(); }
	:	( p=param { $ret.Add($p.ret); } ( COMMA p=param  { $ret.Add($p.ret); } )* )?
	;	
	
// RMI param 
param returns [ Parsed_Param ret ]
@init { $ret = new 	Parsed_Param(); }
	:	pm=paramAttrs 
		varType2=variableType
		varName=IDENTIFIER 
		{ 
			$ret.m_paramAttrs=$pm.ret; 
			$ret.m_type=$varType2.ret;
			$ret.m_name=$varName.text;
		} 
	;
	
variableType returns [ string ret ]
	:
		(
			v1=IDENTIFIER { $ret=$v1.text; }  // e.g. someValue
			| v2=IDENTIFIER DOT child2=variableType { $ret = $v2.text+App.GetMemberSpecificationSymbol()+$child2.ret; } // e.g. obj.someValue
			| v3=IDENTIFIER DOUBLE_COLON child3=variableType { $ret = $v3.text + App.GetMemberSpecificationSymbol() + $child3.ret; } // e.g. obj::someValue
			| v4=IDENTIFIER LT child4=variableTypeTuple GT { $ret = $v4.text + "<" + $child4.ret + ">"; } // e.g. obj<someValue>	
			| v5=IDENTIFIER STAR { $ret += $v5.text+"*"; }  // 포인터 타입
			| v6=IDENTIFIER AMP { $ret += $v6.text+"&"; }  // 리퍼런스 타입
		)
		
		{
			$ret = App.ApplyAlias($ret);
		}
	;
	
variableTypeTuple returns [string ret] // e.g. int someValue
	:
		child=variableType { $ret = $child.text; }
		( 
			COMMA child2=variableType { $ret += "," + $child2.ret; }
		)* 
	;

// RMI param mode
paramAttrs returns [ Parsed_ParamAttrs ret ]
@init { $ret = new Parsed_ParamAttrs(); }
	:	OPEN_BRACKET
		paramAttrVal = paramAttr { $ret.Add($paramAttrVal.ret); } 	
		(
			COMMA paramAttrVal=paramAttr { $ret.Add($paramAttrVal.ret); } 
		) * 
		CLOSE_BRACKET
		{
			var x = $ret.GetInvalidErrorText();
			if(x != null)
			{
				throw new Exception(x);
			}
			$ret.FillDefaults();
		}
	;	

paramAttr returns [ Parsed_ParamAttr ret ]
@init { $ret = new Parsed_ParamAttr(); }
	:
		KW_IN { $ret.m_type = ParamAttrType.In; } |
		KW_BYVAL { $ret.m_type = ParamAttrType.Value; } |
		KW_REF { $ret.m_type = ParamAttrType.Ref; } |
		KW_MUTABLE{ $ret.m_type = ParamAttrType.Mutable; } |
		KW_CONST { $ret.m_type = ParamAttrType.Const; } |
	;

// Java or actionscript Package statement: "package" followed by an identifier.
packageDefinition returns [ Parsed_PackageDef ret ]
@init { $ret = new Parsed_PackageDef(); }
	:	KW_PACKAGE name=variableType { $ret.m_name = $name.ret; }
	SEMICOLON
	;



// modifiers for PIDL classes, interfaces, class/instance vars and methods
modifier returns [ string ret ]
	:	KW_private { $ret = $KW_private.text; }
	  | KW_public { $ret = $KW_public.text; }
		| KW_protected { $ret = $KW_protected.text; }
		| KW_static { $ret = $KW_static.text; }
		| KW_transient { $ret = $KW_transient.text; }
		| KW_final { $ret = $KW_final.text; }
		| KW_abstract { $ret = $KW_abstract.text; }
		| KW_native { $ret = $KW_native.text; }
		| KW_threadsafe { $ret = $KW_threadsafe.text; }
		| KW_synchronized { $ret = $KW_synchronized.text; }
		| KW_volatile { $ret = $KW_volatile.text; }

		;




KW_IMPORT: 'import';
KW_INTERFCE: 'interface';
KW_RENAME: 'rename';
KW_IMOPORT: 'import';
KW_USING: 'using';
KW_INCLUDE: 'include';
KW_GLOBAL: 'global';
KW_COMPONENT: 'component';
KW_DLLEXPORT: 'dllexport';
KW_MODULE: 'module';
KW_MARSHALER: 'marshaler';
KW_ACCESS: 'access';

KW_IN: 		'in';
KW_BYVAL: 		'byval';
KW_REF: 		'ref';
KW_MUTABLE: 		'mutable';
KW_CONST: 		'const';
KW_PACKAGE: 'package';		

KW_private : 'private';
KW_public : 'public';
KW_protected : 'protected';
KW_static : 'static';
KW_transient : 'transient';
KW_final : 'final';
KW_abstract : 'abstract';
KW_native : 'native';
KW_threadsafe : 'threadsafe';
KW_synchronized : 'synchronized';
KW_volatile : 'volatile';


BYTE_ORDER_MARK: '\u00EF\u00BB\u00BF';

// A.1. Documentation Comments
SINGLE_LINE_DOC_COMMENT 
  : ('///' Input_character*) -> channel(HIDDEN)
  ;
DELIMITED_DOC_COMMENT 
  : ('/**' Delimited_comment_section* Asterisks '/') -> channel(HIDDEN) 
  ;

//B.1.1 Line Terminators
NEW_LINE 
  : ('\u000D' //'<Carriage Return Character (U+000D)>'
  | '\u000A' //'<Line Feed Character (U+000A)>'
  | '\u000D' '\u000A' //'<Carriage Return Character (U+000D) Followed By Line Feed Character (U+000A)>'
  | '\u0085' //<Next Line Character (U+0085)>'
  | '\u2028' //'<Line Separator Character (U+2028)>'
  | '\u2029' //'<Paragraph Separator Character (U+2029)>'
  ) -> channel(HIDDEN)
  ;

//B.1.2 Comments
SINGLE_LINE_COMMENT 
  : ('//' Input_character*) -> channel(HIDDEN)
  ;
fragment Input_characters
  : Input_character+
  ;
fragment Input_character 
  : ~([\u000D\u000A\u0085\u2028\u2029]) //'<Any Unicode Character Except A NEW_LINE_CHARACTER>'
  ;
fragment NEW_LINE_CHARACTER 
  : '\u000D' //'<Carriage Return Character (U+000D)>'
  | '\u000A' //'<Line Feed Character (U+000A)>'
  | '\u0085' //'<Next Line Character (U+0085)>'
  | '\u2028' //'<Line Separator Character (U+2028)>'
  | '\u2029' //'<Paragraph Separator Character (U+2029)>'
  ;

DELIMITED_COMMENT 
  : ('/*' Delimited_comment_section* Asterisks '/') -> channel(HIDDEN)
  ;
fragment Delimited_comment_section 
  : '/'
  | Asterisks? Not_slash_or_asterisk
  ;
fragment Asterisks 
  : '*'+
  ;
//'<Any Unicode Character Except / Or *>'
fragment Not_slash_or_asterisk 
  : ~( '/' | '*' )
  ;

//B.1.3 White Space
WHITESPACE 
  : (Whitespace_characters) -> channel(HIDDEN)
  ;

fragment Whitespace_characters 
  : Whitespace_character+
  ;

fragment Whitespace_character 
  : UNICODE_CLASS_ZS //'<Any Character With Unicode Class Zs>'
  | '\u0009' //'<Horizontal Tab Character (U+0009)>'
  | '\u000B' //'<Vertical Tab Character (U+000B)>'
  | '\u000C' //'<Form Feed Character (U+000C)>'
  ;

//B.1.5 Unicode Character Escape Sequences
fragment Unicode_escape_sequence 
  : '\\u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
  | '\\U' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
  ;

//B.1.7 Keywords
//ABSTRACT : 'abstract';
//ADD: 'add';
//ALIAS: 'alias';
//ARGLIST: '__arglist';
//AS : 'as';
//ASCENDING: 'ascending';
//BASE : 'base';
//BOOL : 'bool';
//BREAK : 'break';
//BY: 'by';
//BYTE : 'byte';
//CASE : 'case';
//CATCH : 'catch';
//CHAR : 'char';
//CHECKED : 'checked';
//CLASS : 'class';
//CONST : 'const';
//CONTINUE : 'continue';
//DECIMAL : 'decimal';
//DEFAULT : 'default';
//DELEGATE : 'delegate';
//DESCENDING: 'descending';
//DO : 'do';
//DOUBLE : 'double';
//DYNAMIC: 'dynamic';
//ELSE : 'else';
//ENUM : 'enum';
//EQUALS: 'equals';
//EVENT : 'event';
//EXPLICIT : 'explicit';
//EXTERN : 'extern';
//FALSE : 'false';
//FINALLY : 'finally';
//FIXED : 'fixed';
//FLOAT : 'float';
//FOR : 'for';
//FOREACH : 'foreach';
//FROM: 'from';
//GET: 'get';
//GOTO : 'goto';
//GROUP: 'group';
//IF : 'if';
//IMPLICIT : 'implicit';
//IN : 'in';
//INT : 'int';
//INTERFACE : 'interface';
//INTERNAL : 'internal';
//INTO : 'into';
//IS : 'is';
//JOIN: 'join';
//LET: 'let';
//LOCK : 'lock';
//LONG : 'long';
//NAMESPACE : 'namespace';
//NEW : 'new';
//NULL : 'null';
//OBJECT : 'object';
//ON: 'on';
//OPERATOR : 'operator';
//ORDERBY: 'orderby';
//OUT : 'out';
//OVERRIDE : 'override';
//PARAMS : 'params';
//PARTIAL: 'partial';
//PRIVATE : 'private';
//PROTECTED : 'protected';
//PUBLIC : 'public';
//READONLY : 'readonly';
//REF : 'ref';
//REMOVE: 'remove';
//RETURN : 'return';
//SBYTE : 'sbyte';
//SEALED : 'sealed';
//SELECT: 'select';
//SET: 'set';
//SHORT : 'short';
//SIZEOF : 'sizeof';
//STACKALLOC : 'stackalloc';
//STATIC : 'static';
//STRING : 'string';
//STRUCT : 'struct';
//SWITCH : 'switch';
//THIS : 'this';
//THROW : 'throw';
//TRUE : 'true';
//TRY : 'try';
//TYPEOF : 'typeof';
//UINT : 'uint';
//ULONG : 'ulong';
//UNCHECKED : 'unchecked';
//UNSAFE : 'unsafe';
//USHORT : 'ushort';
//USING : 'using';
//VIRTUAL : 'virtual';
//VOID : 'void';
//VOLATILE : 'volatile';
//WHERE : 'where';
//WHILE : 'while';
//YIELD: 'yield';

//B.1.6 Identifiers
// must be defined after all keywords so the first branch (Available_identifier) does not match keywords 
IDENTIFIER
  : Available_identifier
  | '@' Identifier_or_keyword
  ;
//'<An Identifier_or_keyword That Is Not A Keyword>'
// WARNING: ignores exclusion
fragment Available_identifier 
  : Identifier_or_keyword
  ;
fragment Identifier_or_keyword 
  : Identifier_start_character Identifier_part_character*
  ;
fragment Identifier_start_character 
  : Letter_character
  | '_'
  ;
fragment Identifier_part_character 
  : Letter_character
  | Decimal_digit_character
  | Connecting_character
  | Combining_character
  | Formatting_character
  ;
//'<A Unicode Character Of Classes Lu, Ll, Lt, Lm, Lo, Or Nl>'
// WARNING: ignores Unicode_escape_sequence
fragment Letter_character 
  : UNICODE_CLASS_LU
  | UNICODE_CLASS_LL
  | UNICODE_CLASS_LT
  | UNICODE_CLASS_LM
  | UNICODE_CLASS_LO
  | UNICODE_CLASS_NL
//  | '<A Unicode_escape_sequence Representing A Character Of Classes Lu, Ll, Lt, Lm, Lo, Or Nl>'
  ;
//'<A Unicode Character Of Classes Mn Or Mc>'
// WARNING: ignores Unicode_escape_sequence
fragment Combining_character 
  : UNICODE_CLASS_MN
  | UNICODE_CLASS_MC
//  | '<A Unicode_escape_sequence Representing A Character Of Classes Mn Or Mc>'
  ;
//'<A Unicode Character Of The Class Nd>'
// WARNING: ignores Unicode_escape_sequence
fragment Decimal_digit_character 
  : UNICODE_CLASS_ND
//  | '<A Unicode_escape_sequence Representing A Character Of The Class Nd>'
  ;
//'<A Unicode Character Of The Class Pc>'
// WARNING: ignores Unicode_escape_sequence
fragment Connecting_character 
  : UNICODE_CLASS_PC
//  | '<A Unicode_escape_sequence Representing A Character Of The Class Pc>'
  ;
//'<A Unicode Character Of The Class Cf>'
// WARNING: ignores Unicode_escape_sequence
fragment Formatting_character 
  : UNICODE_CLASS_CF
//  | '<A Unicode_escape_sequence Representing A Character Of The Class Cf>'
  ;

//B.1.8 Literals

INTEGER_LITERAL 
  : Decimal_integer_literal
  | Hexadecimal_integer_literal
  ;
fragment Decimal_integer_literal 
  : Decimal_digits Integer_type_suffix?
  ;
fragment Decimal_digits 
  : DECIMAL_DIGIT+
  ;
fragment DECIMAL_DIGIT 
  : '0'..'9'
  ;
fragment Integer_type_suffix 
  : 'U'
  | 'u'
  | 'L'
  | 'l'
  | 'UL'
  | 'Ul'
  | 'uL'
  | 'ul'
  | 'LU'
  | 'Lu'
  | 'lU'
  | 'lu'
  ;
fragment Hexadecimal_integer_literal 
  : ('0x' | '0X') Hex_digits Integer_type_suffix?
  ;
fragment Hex_digits 
  : HEX_DIGIT+
  ;
fragment HEX_DIGIT 
  : '0'..'9'
  | 'A'..'F'
  | 'a'..'f'
  ;
// added by chw
// For the rare case where 0.ToString() etc is used.
// Explanation: 0.Equals() would be parsed as an invalid real (1. branch) causing a lexer error
LiteralAccess
  : INTEGER_LITERAL   
    DOT               
    IDENTIFIER       
  ;

REAL_LITERAL 
  : Decimal_digits DOT Decimal_digits Exponent_part? Real_type_suffix?
  | DOT Decimal_digits Exponent_part? Real_type_suffix?
  | Decimal_digits Exponent_part Real_type_suffix?
  | Decimal_digits Real_type_suffix
  ;
fragment Exponent_part 
  : ('e' | 'E') Sign? Decimal_digits
  ;
fragment Sign 
  : '+'
  | '-'
  ;
fragment Real_type_suffix 
  : 'F'
  | 'f'
  | 'D'
  | 'd'
  | 'M'
  | 'm'
  ;
CHARACTER_LITERAL 
  : QUOTE Character QUOTE
  ;
fragment Character 
  : Single_character
  | Simple_escape_sequence
  | Hexadecimal_escape_sequence
  | Unicode_escape_sequence
  ;
fragment Single_character 
  : ~(['\\\u000D\u000A\u0085\u2028\u2029]) //'<Any Character Except \' (U+0027), \\ (U+005C), And NEW_LINE_CHARACTER>'
  ;
fragment Simple_escape_sequence 
  : '\\\''
  | '\\"'
  | DOUBLE_BACK_SLASH
  | '\\0'
  | '\\a'
  | '\\b'
  | '\\f'
  | '\\n'
  | '\\r'
  | '\\t'
  | '\\v'
  ;
fragment Hexadecimal_escape_sequence 
  : '\\x' HEX_DIGIT
  | '\\x' HEX_DIGIT HEX_DIGIT
  | '\\x' HEX_DIGIT HEX_DIGIT HEX_DIGIT
  | '\\x' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
  ;
STRING_LITERAL 
  : Regular_string_literal
  | Verbatim_string_literal
  ;
fragment Regular_string_literal 
  : DOUBLE_QUOTE Regular_string_literal_character* DOUBLE_QUOTE
  ;
fragment Regular_string_literal_character 
  : Single_regular_string_literal_character
  | Simple_escape_sequence
  | Hexadecimal_escape_sequence
  | Unicode_escape_sequence
  ;
//'<Any Character Except " (U+0022), \\ (U+005C), And NEW_LINE_CHARACTER>'
fragment Single_regular_string_literal_character 
  : ~(["\\\u000D\u000A\u0085\u2028\u2029])
  ;
fragment Verbatim_string_literal 
  : '@' DOUBLE_QUOTE Verbatim_string_literal_character* DOUBLE_QUOTE
  ;
fragment Verbatim_string_literal_character 
  : Single_verbatim_string_literal_character
  | Quote_escape_sequence
  ;
fragment Single_verbatim_string_literal_character 
  : ~(["]) //<any Character Except ">
  ;
fragment Quote_escape_sequence 
  : DOUBLE_QUOTE DOUBLE_QUOTE
  ;

//B.1.9 Operators And Punctuators
OPEN_BRACE : '{';
CLOSE_BRACE : '}';
OPEN_BRACKET : '[';
CLOSE_BRACKET : ']';
OPEN_PARENS : '(';
CLOSE_PARENS : ')';
OPEN_SNIPPET : '%{';
CLOSE_SNIPPET : '%}';
DOT : '.';
COMMA : ',';
COLON : ':';
SEMICOLON : ';';
PLUS : '+';
MINUS : '-';
STAR : '*';
DIV : '/';
PERCENT : '%';
AMP : '&';
BITWISE_OR : '|';
CARET : '^';
BANG : '!';
TILDE : '~';
ASSIGNMENT : '=';
LT : '<';
GT : '>';
INTERR : '?';
DOUBLE_COLON : '::';
OP_COALESCING : '??';
OP_INC : '++';
OP_DEC : '--';
OP_AND : '&&';
OP_OR : '||';
OP_PTR : '->';
OP_EQ : '==';
OP_NE : '!=';
OP_LE : '<=';
OP_GE : '>=';
OP_ADD_ASSIGNMENT : '+=';
OP_SUB_ASSIGNMENT : '-=';
OP_MULT_ASSIGNMENT : '*=';
OP_DIV_ASSIGNMENT : '/=';
OP_MOD_ASSIGNMENT : '%=';
OP_AND_ASSIGNMENT : '&=';
OP_OR_ASSIGNMENT : '|=';
OP_XOR_ASSIGNMENT : '^=';
OP_LEFT_SHIFT : '<<';
OP_LEFT_SHIFT_ASSIGNMENT : '<<=';

//B.1.10 Pre_processing Directives
// see above

// Custome Lexer rules
QUOTE :             '\'';
DOUBLE_QUOTE :      '"';
BACK_SLASH :        '\\';
DOUBLE_BACK_SLASH : '\\\\';
SHARP :             '#';


//// Unicode character classes
fragment UNICODE_CLASS_ZS
  : '\u0020' // SPACE
  | '\u00A0' // NO_BREAK SPACE
  | '\u1680' // OGHAM SPACE MARK
  | '\u180E' // MONGOLIAN VOWEL SEPARATOR
  | '\u2000' // EN QUAD
  | '\u2001' // EM QUAD
  | '\u2002' // EN SPACE
  | '\u2003' // EM SPACE
  | '\u2004' // THREE_PER_EM SPACE
  | '\u2005' // FOUR_PER_EM SPACE
  | '\u2006' // SIX_PER_EM SPACE
  | '\u2008' // PUNCTUATION SPACE
  | '\u2009' // THIN SPACE
  | '\u200A' // HAIR SPACE
  | '\u202F' // NARROW NO_BREAK SPACE
  | '\u3000' // IDEOGRAPHIC SPACE
  | '\u205F' // MEDIUM MATHEMATICAL SPACE
  ;

fragment UNICODE_CLASS_LU
  : '\u0041'..'\u005A' // LATIN CAPITAL LETTER A_Z
  | '\u00C0'..'\u00DE' // ACCENTED CAPITAL LETTERS
//  | { isUnicodeClass_Lu(Btext) }?
  ;

fragment UNICODE_CLASS_LL
  : '\u0061'..'\u007A' // LATIN SMALL LETTER a_z
  ;

fragment UNICODE_CLASS_LT
  : '\u01C5' // LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON
  | '\u01C8' // LATIN CAPITAL LETTER L WITH SMALL LETTER J
  | '\u01CB' // LATIN CAPITAL LETTER N WITH SMALL LETTER J
  | '\u01F2' // LATIN CAPITAL LETTER D WITH SMALL LETTER Z
  ;

fragment UNICODE_CLASS_LM
  : '\u02B0'..'\u02EE' // MODIFIER LETTERS
  ;

fragment UNICODE_CLASS_LO
  : '\u01BB' // LATIN LETTER TWO WITH STROKE
  | '\u01C0' // LATIN LETTER DENTAL CLICK
  | '\u01C1' // LATIN LETTER LATERAL CLICK
  | '\u01C2' // LATIN LETTER ALVEOLAR CLICK
  | '\u01C3' // LATIN LETTER RETROFLEX CLICK
  | '\u0294' // LATIN LETTER GLOTTAL STOP
  ;

fragment UNICODE_CLASS_NL
  : '\u16EE' // RUNIC ARLAUG SYMBOL
  | '\u16EF' // RUNIC TVIMADUR SYMBOL
  | '\u16F0' // RUNIC BELGTHOR SYMBOL
  | '\u2160' // ROMAN NUMERAL ONE
  | '\u2161' // ROMAN NUMERAL TWO
  | '\u2162' // ROMAN NUMERAL THREE
  | '\u2163' // ROMAN NUMERAL FOUR
  | '\u2164' // ROMAN NUMERAL FIVE
  | '\u2165' // ROMAN NUMERAL SIX
  | '\u2166' // ROMAN NUMERAL SEVEN
  | '\u2167' // ROMAN NUMERAL EIGHT
  | '\u2168' // ROMAN NUMERAL NINE
  | '\u2169' // ROMAN NUMERAL TEN
  | '\u216A' // ROMAN NUMERAL ELEVEN
  | '\u216B' // ROMAN NUMERAL TWELVE
  | '\u216C' // ROMAN NUMERAL FIFTY
  | '\u216D' // ROMAN NUMERAL ONE HUNDRED
  | '\u216E' // ROMAN NUMERAL FIVE HUNDRED
  | '\u216F' // ROMAN NUMERAL ONE THOUSAND
  ;

fragment UNICODE_CLASS_MN
  : '\u0300' // COMBINING GRAVE ACCENT
  | '\u0301' // COMBINING ACUTE ACCENT
  | '\u0302' // COMBINING CIRCUMFLEX ACCENT
  | '\u0303' // COMBINING TILDE
  | '\u0304' // COMBINING MACRON
  | '\u0305' // COMBINING OVERLINE
  | '\u0306' // COMBINING BREVE
  | '\u0307' // COMBINING DOT ABOVE
  | '\u0308' // COMBINING DIAERESIS
  | '\u0309' // COMBINING HOOK ABOVE
  | '\u030A' // COMBINING RING ABOVE
  | '\u030B' // COMBINING DOUBLE ACUTE ACCENT
  | '\u030C' // COMBINING CARON
  | '\u030D' // COMBINING VERTICAL LINE ABOVE
  | '\u030E' // COMBINING DOUBLE VERTICAL LINE ABOVE
  | '\u030F' // COMBINING DOUBLE GRAVE ACCENT
  | '\u0310' // COMBINING CANDRABINDU
  ;

fragment UNICODE_CLASS_MC
  : '\u0903' // DEVANAGARI SIGN VISARGA
  | '\u093E' // DEVANAGARI VOWEL SIGN AA
  | '\u093F' // DEVANAGARI VOWEL SIGN I
  | '\u0940' // DEVANAGARI VOWEL SIGN II
  | '\u0949' // DEVANAGARI VOWEL SIGN CANDRA O
  | '\u094A' // DEVANAGARI VOWEL SIGN SHORT O
  | '\u094B' // DEVANAGARI VOWEL SIGN O
  | '\u094C' // DEVANAGARI VOWEL SIGN AU
  ;

fragment UNICODE_CLASS_CF
  : '\u00AD' // SOFT HYPHEN
  | '\u0600' // ARABIC NUMBER SIGN
  | '\u0601' // ARABIC SIGN SANAH
  | '\u0602' // ARABIC FOOTNOTE MARKER
  | '\u0603' // ARABIC SIGN SAFHA
  | '\u06DD' // ARABIC END OF AYAH
  ;

fragment UNICODE_CLASS_PC
  : '\u005F' // LOW LINE
  | '\u203F' // UNDERTIE
  | '\u2040' // CHARACTER TIE
  | '\u2054' // INVERTED UNDERTIE
  | '\uFE33' // PRESENTATION FORM FOR VERTICAL LOW LINE
  | '\uFE34' // PRESENTATION FORM FOR VERTICAL WAVY LOW LINE
  | '\uFE4D' // DASHED LOW LINE
  | '\uFE4E' // CENTRELINE LOW LINE
  | '\uFE4F' // WAVY LOW LINE
  | '\uFF3F' // FULLWIDTH LOW LINE
  ;

fragment UNICODE_CLASS_ND
  : '\u0030' // DIGIT ZERO
  | '\u0031' // DIGIT ONE
  | '\u0032' // DIGIT TWO
  | '\u0033' // DIGIT THREE
  | '\u0034' // DIGIT FOUR
  | '\u0035' // DIGIT FIVE
  | '\u0036' // DIGIT SIX
  | '\u0037' // DIGIT SEVEN
  | '\u0038' // DIGIT EIGHT
  | '\u0039' // DIGIT NINE
  ;

//fragment UNICODE_CLASS_LU: {}? c=.;
//UNICODE_CLASS_LL
//UNICODE_CLASS_LT
//UNICODE_CLASS_LM
//fragment UNICODE_CLASS_LO: { isUnicodeClass_Lo($c.text) }? c=.;
//UNICODE_CLASS_NL
//UNICODE_CLASS_MN
//UNICODE_CLASS_MC
//UNICODE_CLASS_ND
//UNICODE_CLASS_PC
//UNICODE_CLASS_CF
