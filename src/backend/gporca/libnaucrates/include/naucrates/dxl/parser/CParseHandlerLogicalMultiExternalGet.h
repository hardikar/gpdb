//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerLogicalMultiExternalGet.h
//
//	@doc:
//		SAX parse handler class for parsing logical external get operator node
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerLogicalMultiExternalGet_H
#define GPDXL_CParseHandlerLogicalMultiExternalGet_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalGet.h"

namespace gpdxl
{
	using namespace gpos;


	XERCES_CPP_NAMESPACE_USE

	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerLogicalMultiExternalGet
	//
	//	@doc:
	//		Parse handler for parsing a logical external get operator
	//
	//---------------------------------------------------------------------------
	class CParseHandlerLogicalMultiExternalGet : public CParseHandlerLogicalGet
	{
		private:

			// private copy ctor
			CParseHandlerLogicalMultiExternalGet(const CParseHandlerLogicalMultiExternalGet &);

			// process the start of an element
			virtual
			void StartElement
				(
					const XMLCh* const element_uri, 		// URI of element's namespace
 					const XMLCh* const element_local_name,	// local part of element's name
					const XMLCh* const element_qname,		// element's qname
					const Attributes& attr				// element's attributes
				);

			// process the end of an element
			virtual
			void EndElement
				(
					const XMLCh* const element_uri, 		// URI of element's namespace
					const XMLCh* const element_local_name,	// local part of element's name
					const XMLCh* const element_qname		// element's qname
				);

		public:
			// ctor
			CParseHandlerLogicalMultiExternalGet
				(
				CMemoryPool *mp,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);

	};
}

#endif // !GPDXL_CParseHandlerLogicalMultiExternalGet_H

// EOF
