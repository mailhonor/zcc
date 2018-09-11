/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-11-15
 * ================================
 */

#include "zcc.h"

/* 通过API, 生成如下JSON */
/* {
 * "description": "this is json demo, describe json'apis.",
 * "author": "eli960",
 * "thanks": ["you", "he", 123, true 2.01, null],
 * "APIS": {
 *      "constructor": [
 *          "json()",
 *          "json(std::string &str)",
 *          "json(bool val)"
 *          ],
 *      "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
 *      "object_add_element": "给对象添加一个子节点"
 *   },
 *   "score": 0.98,
 *   "version": 12,
 *   "published": false
 * }
 */

int main()
{
    std::string result;
    zcc::json json;

    json.object_add_element("description", "this is json demo, describe json'apis.");
    
    json.object_add_element("author", new zcc::json("eli960"));

    json.object_add_element("thanks", new zcc::json(), true)
        ->array_add_element("you")
        ->array_add_element("he")
        ->array_add_element(123L)
        ->array_add_element(true)
        ->array_add_element(2.01)
        ->array_add_element(new zcc::json());

    json.object_add_element("APIS", new zcc::json(), true)
        ->object_add_element("constructor", new zcc::json(), true)
        ->array_add_element("json()")
        ->array_add_element("json(std::string &str)")
        ->array_add_element("json(bool val)")
        ->get_parent()
        ->object_add_element("array_add_element", new zcc::json(), true)
        ->array_add_element(new zcc::json("给数组添加一个子节点"))
        ->array_add_element("add a new zcc::json element")
        ->array_add_element("nothing")
        ->get_parent()
        ->object_add_element("object_add_element", "给对象添加一个子节点")
        ->get_parent()
        ->object_add_element("score", 0.98)
        ->object_add_element("version", new zcc::json(12L))
        ->object_add_element("published", false)
        ->object_add_element("published2", false, true)
        ->set_string_value("sssss");


    result.clear();
    json.serialize(result);
    puts(result.c_str());

    json.object_erase_element("APIS");
    result.clear();
    json.serialize(result);
    puts(result.c_str());

    return 0;
}
