#include<jansson.h>
#include<string>
#include<iostream>
using namespace std;

int main(int argc, char* argv[])
{
    string data = "[{\"name\":\"renkj\",\"age\":\"27\", \"sex\":\"male\"}, {\"name\":\"xueni\",\"age\":\"26\", \"sex\":\"female\"}]";
    json_error_t error;
    json_t* test = json_loads(data.c_str(), 0, &error);
    if (json_is_object(test)) {
        //json_t* root = json_object();
        //json_object_set(root, "name", json_object_get(test, "name"));
        //json_object_set(root, "age", json_object_get(test, "age"));
        //json_t* sex = json_object_get(test, "sex");
        //if (sex == NULL){
        //    cout<<"sex is null";
        //}
        //else{
        //    json_object_set(root, "sex", sex);
        //}
        //char* str = json_dumps(root, 0);
        cout<<"json is dict"<<endl;

    }
    else if(json_is_array(test)) {
        cout<<"json array"<<endl;
        size_t index;
        json_t* value;
        json_array_foreach(test, index, value){
            //cout<<"name : "<<json_string_value(json_object_get(value, "name"))<<endl;
            cout<<"name : "<<json_string_value(json_object_get(value, "name")) <<", age: "<<json_string_value(json_object_get(value, "age"))<<", sex: "<<json_string_value(json_object_get(value, "sex"))<<endl;
        }
    }

    json_t* array = json_array();
    json_t* obj = json_object();
    if(json_array_size(array) == 0) {
        json_object_set(obj, "data", json_string(""));
    }
    else{
        json_object_set(obj, "data", array);
    }
    cout<<json_dumps(obj, 0)<<endl;
    return 0;
}
