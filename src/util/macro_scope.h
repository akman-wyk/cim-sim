//
// Created by wyk on 2024/6/19.
//

#pragma once
#include "nlohmann/json.hpp"

namespace cimsim {

#define DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(Type)     \
    friend void to_json(nlohmann::ordered_json&, const Type&); \
    friend void from_json(const nlohmann::ordered_json&, Type&);

#define DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(Type) \
    void to_json(nlohmann::ordered_json&, const Type&);        \
    void from_json(const nlohmann::ordered_json&, Type&);

#define DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(Type, ...) \
    DEFINE_TYPE_TO_JSON_FUNCTION_WITH_DEFAULT(Type, __VA_ARGS__)  \
    DEFINE_TYPE_FROM_JSON_FUNCTION_WITH_DEFAULT(Type, __VA_ARGS__)

#define DEFINE_TYPE_FROM_JSON_FUNCTION_WITH_DEFAULT(Type, ...)                             \
    void from_json(const nlohmann::ordered_json& nlohmann_json_j, Type& nlohmann_json_t) { \
        const Type nlohmann_json_default_obj{};                                            \
        TYPE_FROM_JSON_FIELD_ASSIGN(__VA_ARGS__)                                           \
    }

#define DEFINE_TYPE_TO_JSON_FUNCTION_WITH_DEFAULT(Type, ...)                             \
    void to_json(nlohmann::ordered_json& nlohmann_json_j, const Type& nlohmann_json_t) { \
        TYPE_TO_JSON_FIELD_ASSIGN(__VA_ARGS__)                                           \
    }

#define DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT_INTRUSIVE(Type, ...)                       \
    friend void from_json(const nlohmann::ordered_json& nlohmann_json_j, Type& nlohmann_json_t) { \
        const Type nlohmann_json_default_obj{};                                                   \
        TYPE_FROM_JSON_FIELD_ASSIGN(__VA_ARGS__)                                                  \
    }                                                                                             \
    friend void to_json(nlohmann::ordered_json& nlohmann_json_j, const Type& nlohmann_json_t) {   \
        TYPE_TO_JSON_FIELD_ASSIGN(__VA_ARGS__)                                                    \
    }

#define TYPE_FROM_JSON_FIELD_ASSIGN(...) \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_WITH_DEFAULT, __VA_ARGS__))

#define TYPE_TO_JSON_FIELD_ASSIGN(...) NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))

#define DEFINE_ENUM_FROM_TO_JSON_FUNCTION(EnumType, type1, type2, type_other) \
    void to_json(nlohmann::ordered_json& j, const EnumType& m) {              \
        j = m._to_string();                                                   \
    }                                                                         \
    void from_json(const nlohmann::ordered_json& j, EnumType& m) {            \
        const auto str = j.get<std::string>();                                \
        if (str == #type1) {                                                  \
            m = EnumType::type1;                                              \
        } else if (str == #type2) {                                           \
            m = EnumType::type2;                                              \
        } else {                                                              \
            m = EnumType::type_other;                                         \
        }                                                                     \
    }

#define CIM_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, \
                      _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,  \
                      _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59,  \
                      _60, _61, _62, _63, _64, NAME, ...)                                                             \
    NAME

#define CIM_PASTE1(func, delimiter, v1)     func(v1)
#define CIM_PASTE2(func, delimiter, v1, v2) CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE1(func, delimiter, v2)
#define CIM_PASTE3(func, delimiter, v1, v2, v3) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE2(func, delimiter, v2, v3)
#define CIM_PASTE4(func, delimiter, v1, v2, v3, v4) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE3(func, delimiter, v2, v3, v4)
#define CIM_PASTE5(func, delimiter, v1, v2, v3, v4, v5) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE4(func, delimiter, v2, v3, v4, v5)
#define CIM_PASTE6(func, delimiter, v1, v2, v3, v4, v5, v6) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE5(func, delimiter, v2, v3, v4, v5, v6)
#define CIM_PASTE7(func, delimiter, v1, v2, v3, v4, v5, v6, v7) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE6(func, delimiter, v2, v3, v4, v5, v6, v7)
#define CIM_PASTE8(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE7(func, delimiter, v2, v3, v4, v5, v6, v7, v8)
#define CIM_PASTE9(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE8(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9)
#define CIM_PASTE10(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE9(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define CIM_PASTE11(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) \
    CIM_PASTE1(func, delimiter, v1) delimiter() CIM_PASTE10(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define CIM_PASTE12(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) \
    CIM_PASTE1(func, delimiter, v1)                                                     \
    delimiter() CIM_PASTE11(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define CIM_PASTE13(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) \
    CIM_PASTE1(func, delimiter, v1)                                                          \
    delimiter() CIM_PASTE12(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define CIM_PASTE14(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) \
    CIM_PASTE1(func, delimiter, v1)                                                               \
    delimiter() CIM_PASTE13(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define CIM_PASTE15(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
    CIM_PASTE1(func, delimiter, v1)                                                                    \
    delimiter() CIM_PASTE14(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define CIM_PASTE16(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) \
    CIM_PASTE1(func, delimiter, v1)                                                                         \
    delimiter() CIM_PASTE15(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define CIM_PASTE17(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) \
    CIM_PASTE1(func, delimiter, v1)                                                                              \
    delimiter() CIM_PASTE16(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define CIM_PASTE18(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter()                                                                                                       \
        CIM_PASTE17(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18)
#define CIM_PASTE19(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19)                                                                                              \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter()                                                                                                       \
        CIM_PASTE18(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19)
#define CIM_PASTE20(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20)                                                                                         \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE19(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20)
#define CIM_PASTE21(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21)                                                                                    \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE20(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21)
#define CIM_PASTE22(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22)                                                                               \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE21(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22)
#define CIM_PASTE23(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23)                                                                          \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE22(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23)
#define CIM_PASTE24(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24)                                                                     \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE23(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24)
#define CIM_PASTE25(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25)                                                                \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE24(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25)
#define CIM_PASTE26(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26)                                                           \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE25(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26)
#define CIM_PASTE27(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27)                                                      \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE26(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)
#define CIM_PASTE28(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)                                                 \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE27(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)
#define CIM_PASTE29(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)                                            \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE28(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)
#define CIM_PASTE30(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)                                       \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE29(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)
#define CIM_PASTE31(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)                                  \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE30(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)
#define CIM_PASTE32(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)                             \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE31(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)
#define CIM_PASTE33(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)                        \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE32(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)
#define CIM_PASTE34(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)                   \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE33(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)
#define CIM_PASTE35(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35)              \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE34(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35)
#define CIM_PASTE36(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36)          \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE35(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36)
#define CIM_PASTE37(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37)     \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE36(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37)
#define CIM_PASTE38(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38)                                                                                               \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE37(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38)
#define CIM_PASTE39(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39)                                                                                         \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE38(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39)
#define CIM_PASTE40(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40)                                                                                    \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE39(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40)
#define CIM_PASTE41(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41)                                                                               \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE40(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41)
#define CIM_PASTE42(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42)                                                                          \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE41(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42)
#define CIM_PASTE43(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43)                                                                     \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE42(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43)
#define CIM_PASTE44(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44)                                                                \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE43(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44)
#define CIM_PASTE45(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45)                                                           \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE44(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45)
#define CIM_PASTE46(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46)                                                      \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE45(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46)
#define CIM_PASTE47(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)                                                 \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE46(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)
#define CIM_PASTE48(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)                                            \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE47(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)
#define CIM_PASTE49(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)                                       \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE48(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)
#define CIM_PASTE50(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)                                  \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE49(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)
#define CIM_PASTE51(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)                             \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE50(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)
#define CIM_PASTE52(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52)                        \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE51(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52)
#define CIM_PASTE53(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53)                   \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE52(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53)
#define CIM_PASTE54(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54)               \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE53(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54)
#define CIM_PASTE55(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55)          \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE54(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55)
#define CIM_PASTE56(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56)     \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE55(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56)
#define CIM_PASTE57(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,     \
                    v57)                                                                                               \
    CIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        CIM_PASTE56(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57)
#define CIM_PASTE58(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58)                                                                                         \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE57(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58)
#define CIM_PASTE59(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59)                                                                                    \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE58(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59)
#define CIM_PASTE60(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60)                                                                               \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE59(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60)
#define CIM_PASTE61(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61)                                                                          \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE60(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61)
#define CIM_PASTE62(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61, v62)                                                                     \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE61(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61, v62)
#define CIM_PASTE63(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61, v62, v63)                                                                \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE62(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61, v62, v63)
#define CIM_PASTE64(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61, v62, v63, v64)                                                           \
    CIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() CIM_PASTE63(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61, v62, v63, v64)

#define CIM_PASTE(func, delimiter, ...)                                                                             \
    CIM_GET_MACRO(                                                                                                  \
        __VA_ARGS__, CIM_PASTE64, CIM_PASTE63, CIM_PASTE62, CIM_PASTE61, CIM_PASTE60, CIM_PASTE59, CIM_PASTE58,     \
        CIM_PASTE57, CIM_PASTE56, CIM_PASTE55, CIM_PASTE54, CIM_PASTE53, CIM_PASTE52, CIM_PASTE51, CIM_PASTE50,     \
        CIM_PASTE49, CIM_PASTE48, CIM_PASTE47, CIM_PASTE46, CIM_PASTE45, CIM_PASTE44, CIM_PASTE43, CIM_PASTE42,     \
        CIM_PASTE41, CIM_PASTE40, CIM_PASTE39, CIM_PASTE38, CIM_PASTE37, CIM_PASTE36, CIM_PASTE35, CIM_PASTE34,     \
        CIM_PASTE33, CIM_PASTE32, CIM_PASTE31, CIM_PASTE30, CIM_PASTE29, CIM_PASTE28, CIM_PASTE27, CIM_PASTE26,     \
        CIM_PASTE25, CIM_PASTE24, CIM_PASTE23, CIM_PASTE22, CIM_PASTE21, CIM_PASTE20, CIM_PASTE19, CIM_PASTE18,     \
        CIM_PASTE17, CIM_PASTE16, CIM_PASTE15, CIM_PASTE14, CIM_PASTE13, CIM_PASTE12, CIM_PASTE11, CIM_PASTE10,     \
        CIM_PASTE9, CIM_PASTE8, CIM_PASTE7, CIM_PASTE6, CIM_PASTE5, CIM_PASTE4, CIM_PASTE3, CIM_PASTE2, CIM_PASTE1) \
    (func, delimiter, __VA_ARGS__)

#define DELIMITER_COMMA() ,
#define DELIMITER_AND()   &&
#define DELIMITER_SPACE()

#define CIM_PAYLOAD_CONSTRUCTOR_DEFINE_PARAMETER(field)  decltype(field) field
#define CIM_PAYLOAD_CONSTRUCTOR_ASSIGN_MEMBER(field)     field(std::move(field))
#define CIM_PAYLOAD_COPY_CONSTRUCTOR_MEMBER(field)       field(another.field)
#define CIM_PAYLOAD_EQUAL_OPERATOR_COMPARE_MEMBER(field) field == another.field
#define CIM_PAYLOAD_TO_STRING_FUNCTION_MEMBER(field) \
    ss << #field << ": ";                            \
    ss << (field) << "\n";

#define DECLARE_CIM_PAYLOAD_FUNCTIONS(Type)     \
    Type() = default;                           \
    bool operator==(const Type& another) const; \
    [[nodiscard]] std::string toString() const;

#define DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(Type, ...)                                             \
    bool Type::operator==(const Type& another) const {                                           \
        return CIM_PASTE(CIM_PAYLOAD_EQUAL_OPERATOR_COMPARE_MEMBER, DELIMITER_AND, __VA_ARGS__); \
    }

#define DEFINE_CIM_PAYLOAD_TO_STRING_FUNCTION(Type, ...)                               \
    std::string Type::toString() const {                                               \
        std::stringstream ss;                                                          \
        CIM_PASTE(CIM_PAYLOAD_TO_STRING_FUNCTION_MEMBER, DELIMITER_SPACE, __VA_ARGS__) \
        return ss.str();                                                               \
    }

#define DEFINE_CIM_PAYLOAD_FUNCTIONS(Type, ...)          \
    DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(Type, __VA_ARGS__) \
    DEFINE_CIM_PAYLOAD_TO_STRING_FUNCTION(Type, __VA_ARGS__)

#define DEFINE_EXECUTE_INS_PAYLOAD_CONSTRUCTOR(Type, ...)                                                           \
    Type() = default;                                                                                               \
    Type(InstructionPayload ins, CIM_PASTE(CIM_PAYLOAD_CONSTRUCTOR_DEFINE_PARAMETER, DELIMITER_COMMA, __VA_ARGS__)) \
        : ExecuteInsPayload(ins), CIM_PASTE(CIM_PAYLOAD_CONSTRUCTOR_ASSIGN_MEMBER, DELIMITER_COMMA, __VA_ARGS__) {} \
    Type(const Type& another)                                                                                       \
        : ExecuteInsPayload(another.ins)                                                                            \
        , CIM_PASTE(CIM_PAYLOAD_COPY_CONSTRUCTOR_MEMBER, DELIMITER_COMMA, __VA_ARGS__) {}

#define DEFINE_EXECUTE_INS_PAYLOAD_EQUAL_OPERATOR(Type, ...)                                     \
    bool operator==(const Type& another) const {                                                 \
        return CIM_PASTE(CIM_PAYLOAD_EQUAL_OPERATOR_COMPARE_MEMBER, DELIMITER_AND, __VA_ARGS__); \
    }

#define DEFINE_EXECUTE_INS_PAYLOAD_TO_STRING_FUNCTION(Type, ...)                       \
    std::string toString() const {                                                     \
        std::stringstream ss;                                                          \
        CIM_PASTE(CIM_PAYLOAD_TO_STRING_FUNCTION_MEMBER, DELIMITER_SPACE, __VA_ARGS__) \
        return ss.str();                                                               \
    }

#define DEFINE_EXECUTE_INS_PAYLOAD_FUNCTIONS(Type, ins, ...)          \
    DEFINE_EXECUTE_INS_PAYLOAD_CONSTRUCTOR(Type, __VA_ARGS__)         \
    DEFINE_EXECUTE_INS_PAYLOAD_EQUAL_OPERATOR(Type, ins, __VA_ARGS__) \
    DEFINE_EXECUTE_INS_PAYLOAD_TO_STRING_FUNCTION(Type, ins, __VA_ARGS__)

#define MAKE_SIGNAL_TYPE_TRACE_STREAM(CLASS_NAME)                                \
    friend std::ostream& operator<<(std::ostream& out, const CLASS_NAME& self) { \
        out << " " #CLASS_NAME " Type\n";                                        \
        return out;                                                              \
    }                                                                            \
    inline friend void sc_trace(sc_core::sc_trace_file* f, const CLASS_NAME& self, const std::string& name) {}

}  // namespace cimsim
