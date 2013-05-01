#ifndef FUNC_MACROS_H_
#define FUNC_MACROS_H_

#define GET_INST_OPR_I(attr,opr,t) \
	switch(attr) \
	{ \
		case MATTR_SB: \
		case MATTR_UB: \
			(opr).val = *((uint8_t*)(t)); \
			(t) += SIZE_F; \
			break; \
		case MATTR_SF: \
		case MATTR_UF: \
		case MATTR_FS: \
			(opr).val = *((uint32_t*)(t)); \
			(t) += SIZE_F; \
			break; \
		case MATTR_SE: \
		case MATTR_UE: \
		case MATTR_FD: \
			(opr).val = *((uint64_t*)(t)); \
			(t) += SIZE_E; \
			break; \
		default: \
			return 0;\
	}


#define GET_INST_OPR_M(addrm,opr,t) \
	switch(addrm) { \
		case ADDRM_ABSOLUTE: \
		case ADDRM_INDIRECT: \
			(opr).val = *((uint64_t*)(t));\
			(t) += SIZE_E; \
			break; \
		case ADDRM_DISPLACEMENT: \
			(opr).disp32 = *((uint32_t*)(t)); \
			(t) += SIZE_F; \
			(opr).val = *((uint64_t*)(t)); \
			(t) += SIZE_E; \
			break; \
		default: \
			return 0; \
	}

#define GET_INST_OPR_D(addrm,attr,opr,t) \
	switch(addrm) \
	{ \
		case ADDRM_IMMEDIATE: \
			switch(attr) \
			{ \
				case MATTR_SB: \
				case MATTR_UB: \
					(opr).val = *((uint8_t*)(t)); \
					(t) += SIZE_F; \
					break; \
				case MATTR_SF: \
				case MATTR_UF: \
				case MATTR_FS: \
					(opr).val = *((uint32_t*)(t)); \
					(t) += SIZE_F; \
					break; \
				case MATTR_SE: \
				case MATTR_UE: \
				case MATTR_FD: \
					(opr).val = *((uint64_t*)(t)); \
					(t) += SIZE_E; \
					break; \
				default: \
					return 0;\
			} \
			break; \
		case ADDRM_ABSOLUTE: \
		case ADDRM_INDIRECT: \
			(opr).val = *((uint64_t*)(t));\
			(t) += SIZE_E; \
			break; \
		case ADDRM_DISPLACEMENT: \
			(opr).disp32 = *((uint32_t*)(t)); \
			(t) += SIZE_F; \
			(opr).val = *((uint64_t*)(t)); \
			(t) += SIZE_E; \
			break; \
		default: \
			return 0; \
	}

#endif /* FUNC_MACROS_H_ */
