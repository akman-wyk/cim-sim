//
// Created by wyk on 2024/6/19.
//

#pragma once
#include "nlohmann/json.hpp"

namespace pimsim {

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

#define PIM_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, \
                      _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,  \
                      _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59,  \
                      _60, _61, _62, _63, _64, NAME, ...)                                                             \
    NAME

#define PIM_PASTE1(func, delimiter, v1)     func(v1)
#define PIM_PASTE2(func, delimiter, v1, v2) PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE1(func, delimiter, v2)
#define PIM_PASTE3(func, delimiter, v1, v2, v3) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE2(func, delimiter, v2, v3)
#define PIM_PASTE4(func, delimiter, v1, v2, v3, v4) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE3(func, delimiter, v2, v3, v4)
#define PIM_PASTE5(func, delimiter, v1, v2, v3, v4, v5) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE4(func, delimiter, v2, v3, v4, v5)
#define PIM_PASTE6(func, delimiter, v1, v2, v3, v4, v5, v6) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE5(func, delimiter, v2, v3, v4, v5, v6)
#define PIM_PASTE7(func, delimiter, v1, v2, v3, v4, v5, v6, v7) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE6(func, delimiter, v2, v3, v4, v5, v6, v7)
#define PIM_PASTE8(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE7(func, delimiter, v2, v3, v4, v5, v6, v7, v8)
#define PIM_PASTE9(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE8(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9)
#define PIM_PASTE10(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE9(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define PIM_PASTE11(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) \
    PIM_PASTE1(func, delimiter, v1) delimiter() PIM_PASTE10(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define PIM_PASTE12(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) \
    PIM_PASTE1(func, delimiter, v1)                                                     \
    delimiter() PIM_PASTE11(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define PIM_PASTE13(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) \
    PIM_PASTE1(func, delimiter, v1)                                                          \
    delimiter() PIM_PASTE12(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define PIM_PASTE14(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) \
    PIM_PASTE1(func, delimiter, v1)                                                               \
    delimiter() PIM_PASTE13(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define PIM_PASTE15(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
    PIM_PASTE1(func, delimiter, v1)                                                                    \
    delimiter() PIM_PASTE14(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define PIM_PASTE16(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) \
    PIM_PASTE1(func, delimiter, v1)                                                                         \
    delimiter() PIM_PASTE15(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define PIM_PASTE17(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) \
    PIM_PASTE1(func, delimiter, v1)                                                                              \
    delimiter() PIM_PASTE16(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define PIM_PASTE18(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter()                                                                                                       \
        PIM_PASTE17(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18)
#define PIM_PASTE19(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19)                                                                                              \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter()                                                                                                       \
        PIM_PASTE18(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19)
#define PIM_PASTE20(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20)                                                                                         \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE19(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20)
#define PIM_PASTE21(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21)                                                                                    \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE20(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21)
#define PIM_PASTE22(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22)                                                                               \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE21(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22)
#define PIM_PASTE23(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23)                                                                          \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE22(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23)
#define PIM_PASTE24(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24)                                                                     \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE23(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24)
#define PIM_PASTE25(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25)                                                                \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE24(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25)
#define PIM_PASTE26(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26)                                                           \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE25(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26)
#define PIM_PASTE27(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27)                                                      \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE26(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)
#define PIM_PASTE28(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)                                                 \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE27(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)
#define PIM_PASTE29(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)                                            \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE28(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)
#define PIM_PASTE30(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)                                       \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE29(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)
#define PIM_PASTE31(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)                                  \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE30(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)
#define PIM_PASTE32(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)                             \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE31(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)
#define PIM_PASTE33(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)                        \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE32(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)
#define PIM_PASTE34(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)                   \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE33(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)
#define PIM_PASTE35(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35)              \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE34(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35)
#define PIM_PASTE36(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36)          \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE35(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36)
#define PIM_PASTE37(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37)     \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE36(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37)
#define PIM_PASTE38(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38)                                                                                               \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE37(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38)
#define PIM_PASTE39(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39)                                                                                         \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE38(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39)
#define PIM_PASTE40(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40)                                                                                    \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE39(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40)
#define PIM_PASTE41(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41)                                                                               \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE40(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41)
#define PIM_PASTE42(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42)                                                                          \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE41(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42)
#define PIM_PASTE43(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43)                                                                     \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE42(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43)
#define PIM_PASTE44(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44)                                                                \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE43(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44)
#define PIM_PASTE45(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45)                                                           \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE44(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45)
#define PIM_PASTE46(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46)                                                      \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE45(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46)
#define PIM_PASTE47(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)                                                 \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE46(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)
#define PIM_PASTE48(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)                                            \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE47(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)
#define PIM_PASTE49(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)                                       \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE48(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)
#define PIM_PASTE50(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)                                  \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE49(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)
#define PIM_PASTE51(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)                             \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE50(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)
#define PIM_PASTE52(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52)                        \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE51(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52)
#define PIM_PASTE53(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53)                   \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE52(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53)
#define PIM_PASTE54(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54)               \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE53(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54)
#define PIM_PASTE55(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55)          \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE54(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55)
#define PIM_PASTE56(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56)     \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE55(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56)
#define PIM_PASTE57(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,  \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,     \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,     \
                    v57)                                                                                               \
    PIM_PASTE1(func, delimiter, v1)                                                                                    \
    delimiter()                                                                                                        \
        PIM_PASTE56(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, \
                    v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,     \
                    v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57)
#define PIM_PASTE58(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58)                                                                                         \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE57(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58)
#define PIM_PASTE59(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59)                                                                                    \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE58(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59)
#define PIM_PASTE60(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60)                                                                               \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE59(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60)
#define PIM_PASTE61(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61)                                                                          \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE60(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61)
#define PIM_PASTE62(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61, v62)                                                                     \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE61(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61, v62)
#define PIM_PASTE63(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61, v62, v63)                                                                \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE62(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61, v62, v63)
#define PIM_PASTE64(func, delimiter, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, \
                    v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,    \
                    v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56,    \
                    v57, v58, v59, v60, v61, v62, v63, v64)                                                           \
    PIM_PASTE1(func, delimiter, v1)                                                                                   \
    delimiter() PIM_PASTE63(func, delimiter, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,  \
                            v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, \
                            v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, \
                            v54, v55, v56, v57, v58, v59, v60, v61, v62, v63, v64)

#define PIM_PASTE(func, delimiter, ...)                                                                             \
    PIM_GET_MACRO(                                                                                                  \
        __VA_ARGS__, PIM_PASTE64, PIM_PASTE63, PIM_PASTE62, PIM_PASTE61, PIM_PASTE60, PIM_PASTE59, PIM_PASTE58,     \
        PIM_PASTE57, PIM_PASTE56, PIM_PASTE55, PIM_PASTE54, PIM_PASTE53, PIM_PASTE52, PIM_PASTE51, PIM_PASTE50,     \
        PIM_PASTE49, PIM_PASTE48, PIM_PASTE47, PIM_PASTE46, PIM_PASTE45, PIM_PASTE44, PIM_PASTE43, PIM_PASTE42,     \
        PIM_PASTE41, PIM_PASTE40, PIM_PASTE39, PIM_PASTE38, PIM_PASTE37, PIM_PASTE36, PIM_PASTE35, PIM_PASTE34,     \
        PIM_PASTE33, PIM_PASTE32, PIM_PASTE31, PIM_PASTE30, PIM_PASTE29, PIM_PASTE28, PIM_PASTE27, PIM_PASTE26,     \
        PIM_PASTE25, PIM_PASTE24, PIM_PASTE23, PIM_PASTE22, PIM_PASTE21, PIM_PASTE20, PIM_PASTE19, PIM_PASTE18,     \
        PIM_PASTE17, PIM_PASTE16, PIM_PASTE15, PIM_PASTE14, PIM_PASTE13, PIM_PASTE12, PIM_PASTE11, PIM_PASTE10,     \
        PIM_PASTE9, PIM_PASTE8, PIM_PASTE7, PIM_PASTE6, PIM_PASTE5, PIM_PASTE4, PIM_PASTE3, PIM_PASTE2, PIM_PASTE1) \
    (func, delimiter, __VA_ARGS__)

#define DELIMITER_COMMA() ,
#define DELIMITER_AND()   &&
#define DELIMITER_SPACE()

#define PIM_PAYLOAD_CONSTRUCTOR_DEFINE_PARAMETER(field)  decltype(field) field
#define PIM_PAYLOAD_CONSTRUCTOR_ASSIGN_MEMBER(field)     field(std::move(field))
#define PIM_PAYLOAD_COPY_CONSTRUCTOR_MEMBER(field)       field(another.field)
#define PIM_PAYLOAD_EQUAL_OPERATOR_COMPARE_MEMBER(field) field == another.field
#define PIM_PAYLOAD_TO_STRING_FUNCTION_MEMBER(field) \
    ss << #field << ": ";                            \
    ss << (field) << "\n";

#define DECLARE_PIM_PAYLOAD_FUNCTIONS(Type)     \
    Type() = default;                           \
    bool operator==(const Type& another) const; \
    [[nodiscard]] std::string toString() const;

#define DEFINE_PIM_PAYLOAD_EQUAL_OPERATOR(Type, ...)                                             \
    bool Type::operator==(const Type& another) const {                                           \
        return PIM_PASTE(PIM_PAYLOAD_EQUAL_OPERATOR_COMPARE_MEMBER, DELIMITER_AND, __VA_ARGS__); \
    }

#define DEFINE_PIM_PAYLOAD_TO_STRING_FUNCTION(Type, ...)                               \
    std::string Type::toString() const {                                               \
        std::stringstream ss;                                                          \
        PIM_PASTE(PIM_PAYLOAD_TO_STRING_FUNCTION_MEMBER, DELIMITER_SPACE, __VA_ARGS__) \
        return ss.str();                                                               \
    }

#define DEFINE_PIM_PAYLOAD_FUNCTIONS(Type, ...)          \
    DEFINE_PIM_PAYLOAD_EQUAL_OPERATOR(Type, __VA_ARGS__) \
    DEFINE_PIM_PAYLOAD_TO_STRING_FUNCTION(Type, __VA_ARGS__)

#define DEFINE_EXECUTE_INS_PAYLOAD_CONSTRUCTOR(Type, ...)                                                           \
    Type() = default;                                                                                               \
    Type(InstructionPayload ins, PIM_PASTE(PIM_PAYLOAD_CONSTRUCTOR_DEFINE_PARAMETER, DELIMITER_COMMA, __VA_ARGS__)) \
        : ExecuteInsPayload(ins), PIM_PASTE(PIM_PAYLOAD_CONSTRUCTOR_ASSIGN_MEMBER, DELIMITER_COMMA, __VA_ARGS__) {} \
    Type(const Type& another)                                                                                       \
        : ExecuteInsPayload(another.ins)                                                                            \
        , PIM_PASTE(PIM_PAYLOAD_COPY_CONSTRUCTOR_MEMBER, DELIMITER_COMMA, __VA_ARGS__) {}

#define DEFINE_EXECUTE_INS_PAYLOAD_EQUAL_OPERATOR(Type, ...)                                     \
    bool operator==(const Type& another) const {                                                 \
        return PIM_PASTE(PIM_PAYLOAD_EQUAL_OPERATOR_COMPARE_MEMBER, DELIMITER_AND, __VA_ARGS__); \
    }

#define DEFINE_EXECUTE_INS_PAYLOAD_TO_STRING_FUNCTION(Type, ...)                       \
    std::string toString() const {                                                     \
        std::stringstream ss;                                                          \
        PIM_PASTE(PIM_PAYLOAD_TO_STRING_FUNCTION_MEMBER, DELIMITER_SPACE, __VA_ARGS__) \
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

}  // namespace pimsim
