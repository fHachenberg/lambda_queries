#include <functional>
#include <set>
#include <map>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <numeric>

/* Dieser Code repräsentiert eine Datenbank von GUID->Flidx sowie Queries, die
 * einzelne Einträge aus dieser Datenbank rausholen können. Die Queries können durch
 * ListenQueries zu größeren Ausdrücken zusammengesetzt werden. Es können Namen für
 * Queries vergeben werden ("Gruppen") und diese in eigenen Queries referenziert werden.
 * 
 * Fazit: So kann man es mit sehr wenig Code machen, aber dieser Ansatz taugt eher für
 * eine Konstruktion zur Compilezeit oder einmalig zur Laufzeit. Es fehlt die Möglichkeit
 * zur Introspektion, wir können z.Bsp. keinen BAUM von Queries abfragen.
 * 
 * Deswegen sollte man letztlich doch mit Query-Klassen arbeiten. Das muss man dann
 * auf dem Heap tun, in diesem Code haben wir das Thema "heap" elegant dadurch ausgespart,
 * dass wir std::function-Objekte herumgereicht haben, die sich selber darum kümmern.
 * 
 * Wenn die Gruppen später mächtiger werden, kann man übergehen von
 *  "eine Gruppe IST ein Query" zu "eine Gruppe HAT ein Query"
 * */

typedef int Flidx;
typedef std::set<Flidx> FlidxSet;

typedef FlidxSet QueryType(); //Typ der Query-Objekte

typedef int GUID;

typedef std::string GroupLabel;
typedef std::function<QueryType> Query;
typedef std::shared_ptr<Query> QueryPtr;

typedef std::map<GUID, Flidx> GUIDDatabase; //Datenbank GUID -> Flächennummer
typedef std::map<GroupLabel, Query> GroupDatabase; //Datenbank Gruppenlabel -> Query

//Erzeugt eine Menge von Flächenindices aus 
FlidxSet makeFlidxSet(Flidx const& single) { FlidxSet res; res.insert(single); return res; }
FlidxSet makeFlidxSet(Flidx const& first, Flidx const& last) 
{ 
    FlidxSet res; 
    std::vector<int> vals(last-first+1); 
    std::iota(vals.begin(), vals.end(), first);    
    res.insert(vals.begin(), vals.end());    
    return res;
}

class QueryContext
{
    private:
    GUIDDatabase const& guid_db;
    GroupDatabase const& group_db;
    
    public:
    QueryContext(GUIDDatabase const& a_guid_db, GroupDatabase const& a_group_db) : guid_db(a_guid_db), group_db(a_group_db) {}
    
    Query createQuery(GUID const& guid)
    {
        auto guid_query = [this, guid]() { return makeFlidxSet(this->guid_db.find(guid)->second); };        
        return static_cast<Query>(guid_query);
    }
    
    Query createQuery(GUID const& first, GUID const& last)
    {
        auto guidrange_query = [this, first, last]() { return makeFlidxSet(this->guid_db.find(first)->second, this->guid_db.find(last)->second); };        
        return static_cast<Query>(guidrange_query);
    }
    
    Query createQuery(GroupLabel const& label)
    {
        auto label_query = [this, label]() { auto query = this->group_db.find(label)->second; return query(); };        
        return static_cast<Query>(label_query);
    }
    
    Query createQuery(std::vector<Query> const& queries)
    {
        auto list_query = [this, queries]() 
        { 
            FlidxSet combi_res;
            for(auto query : queries)
            {
                FlidxSet query_res = query();
                combi_res.insert(query_res.begin(), query_res.end());
            }
            return combi_res;
        };
        return static_cast<Query>(list_query);
    }
};

int main()
{
    GUIDDatabase guid_db = {{0, 0}, {16, 1}, {32, 2}, {64, 3}};
    GroupDatabase group_db;
    
    auto factory = QueryContext(guid_db, group_db);
    auto query = factory.createQuery(0);
    group_db["otto"] = query;    
    auto groupquery = factory.createQuery("otto");    
    auto range_query = factory.createQuery(0, 32);
    std::cout << range_query().size() << std::endl;
    
    auto list_query = factory.createQuery({groupquery, groupquery, factory.createQuery(16), factory.createQuery(32), factory.createQuery(64)});
    
    std::cout << list_query().size() << std::endl;
}
