/**
 * Classes providing support for XML data files. These classes
 * implement only those aspects of XML required to read, write, and
 * manipulate CTML data files.
 */

#ifndef CT_XML
#define CT_XML

#include <string>
#include <vector>
#include <iostream>
using namespace std;

#include "ctexceptions.h"
#include "ct_defs.h"
#include "stringUtils.h"
#include <stdio.h>
#include "global.h"

#define XML_INDENT 4


namespace Cantera {


    /**
     * Class XML_Reader is designed for internal use.
     */
    class XML_Reader {
    public:
        XML_Reader(istream& input) : m_s(input), m_line(0) {}

        istream& m_s;
        int m_line;

        void getchr(char& ch);
        string strip(const string& aline);
        string inquotes(const string& aline);
        void parseTag(string line, string& name, map<string, string>& attribs);
        string readTag(map<string, string>& attribs);
        string readValue();
    };


    //////////////////////////  XML_Node  /////////////////////////////////

    class XML_Node {
    public:

        XML_Node(string nm = "--", XML_Node* p = 0, int n = 0);
        virtual ~XML_Node();
        void addComment(string comment);
        XML_Node& addChild(XML_Node& node);
        XML_Node& addChild(string name);
        XML_Node& addChild(string name, string value);
        XML_Node& addChild(string name, double value, string fmt="%g");
        void removeChild(XML_Node* node);
        void addValue(string val);
        void addValue(doublereal val, string fmt="%g");
        void addAttribute(string attrib, string value);
        void addAttribute(string attrib, double value, string fmt="%g");
        void writeHeader(ostream& s);
        string value() const { return m_value; }
        string value(string loc) const { return child(loc).value(); }
        doublereal fp_value() const { 
            return atof(m_value.c_str()); 
        }
        integer int_value() const { 
            return atoi(m_value.c_str()); 
        }
        string operator()() const { return m_value; }
        string operator()(string loc) const { return value(loc); }
        string operator[](string attr) const {
            return attrib(attr);
            //if (hasAttrib(attr)) {return m_attribs[attr]; }
        }
        string attrib(string attr) const { 
            map<string,string>::const_iterator i = m_attribs.find(attr);
            if (i != m_attribs.end()) return i->second;
            else return ""; 
        }
        map<string,string>& attribs() { return m_attribs; }
        XML_Node* parent() { return m_parent; }
        XML_Node* setParent(XML_Node* p) { m_parent = p; return p; }

        bool hasChild(string ch) {
            return (m_childindex.find(ch) != m_childindex.end());
            //return (m_childindex[ch] != 0);
        }
        bool hasAttrib(string a) const {
            return (m_attribs.find(a) != m_attribs.end());
            //if (m_attribs[a] != "") return true;
            //else {
            //    m_attribs.erase(a);
            //    return false;
            //}
        }

        string name() { return m_name; }
        string id() {
            if (hasAttrib("id")) return attrib("id");
            else return "";
        }
        int number() { return m_n; }

        XML_Node& child(int n) const { return *m_children[n]; }
        vector<XML_Node*> children() { return m_children; }
        int nChildren() const { return m_nchildren; }

        void build(istream& f);

        XML_Node* findID(const string& id, int depth=100);
        XML_Node* findByAttr(const string& attr, const string& val);
        XML_Node* findByName(const string& nm);
        void getChildren(string name, vector<XML_Node*>& children) const;
        XML_Node& child(string loc) const;
        void write(ostream& s, int level = 0);
        XML_Node* getRef();
        XML_Node& root() { return *m_root; }
        void setRoot(XML_Node& root) { m_root = &root; }

    protected:

        vector<string> m_tags;
        string m_name;
        string m_value;
        int m_level;
        map<string, XML_Node*> m_childindex;
        map<string, string> m_attribs;
        XML_Node* m_parent;
        XML_Node* m_root;
        vector<XML_Node*> m_children;
        int m_nchildren;
        int m_n;
        bool m_iscomment;
    };


    //////////////////////  XML_Writer  //////////////////////////

    class XML_Writer {
    public:
        XML_Writer(ostream& output) : 
	    m_s(output), _indent("   "), _level(0) {}
        virtual ~XML_Writer() {}
        ostream& m_s;

        string _indent;
        int _level;

        ostream& output() { return m_s; }

        inline string XML_filter(string name) { 
            int ns = name.size();
            string nm(name);
            for (int m = 0; m < ns; m++) 
                if (name[m] == ' ' 
                    || name[m] == '('
                    || name[m] == ')') 
                    nm[m] = '_';
            return nm;
        }

        /**
	 * XML_comment()
	 *
	 *  Add a comment element to the current XML output file
	 *  Comment elements start with <!-- and end with -->
	 *  Comments are indented according to the current lvl,
	 *  _level
	 *
	 *  input
	 * ---------
	 *    s : Output stream containing the XML file
	 *    comment : Reference to a string containing the comment
	 */
        inline void XML_comment(ostream& s, const string& comment) {
            for (int n = 0; n < _level; n++) s << _indent;
            s << "<!--" << comment << "-->" << endl;
        }

        inline void XML_open(ostream& s, const string& tag, const string p = "") {
            for (int n = 0; n < _level; n++) s << _indent;
            _level++;
            s << "<" << XML_filter(tag) << p << ">" << endl;
        }

        inline void XML_close(ostream& s, const string& tag) {
            _level--;
            for (int n = 0; n < _level; n++) s << _indent;
            s << "</" << XML_filter(tag) << ">" << endl;
        }

        template<class T>
        void XML_item(ostream& s, const string& tag, T value) {
            for (int n = 0; n < _level; n++) s << _indent;
            s << "<" << XML_filter(tag) << ">" 
              << value << "</" << tag << ">" << endl;
        }

        template<class iter>
        void XML_writeVector(ostream& s, const string& indent, 
            const string& name, int vsize, iter v) {
            int ni;
            for (ni = 0; ni < _level; ni++) s << _indent;
            s << "<" << XML_filter(name) << "> ";
            
            int n = vsize;
            int n5 = n/5;
            int i, j, k = 0;
            for (j = 0; j < n5; j++) {
                for (i = 0; i < 5; i++) {
                    s << v[k] << (k < n - 1 ? ", " : ""); 
                    k++;
                }
                if (j < n5-1) {
                    s << endl;
                    for (ni = 0; ni < _level; ni++) s << _indent;
                }
            }
            for (i = k; i < n; i++) { 
                s << v[k] << (k < n - 1 ? ", " : ""); 
                k++;
            }
            
            s << "</" << XML_filter(name) << ">" << endl;
        }
    };

    //inline XML_getByID(string file, string id) {

    XML_Node* find_XML(string src, XML_Node* root=0, 
        string id="", string loc="", string name="");
        
}

#endif

