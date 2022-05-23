#include "utils.h"
#include <tuple>
#include <iostream>
#include <cstdint> // include this header for uint64_t
#include <cstring>
#include <unordered_map>
 
using namespace std;

int tot_movements=0;
int verbose=0;

template <typename key_type, typename value_type> class HTmap;  // forward declare
template <typename key_type, typename value_type> class HTIterator; // forward declare

template <typename key_type, typename value_type> class HTmap {
		bool        ***present_table;    //  present flag memory
		pair<key_type,value_type>  ***table;      // entries stored in the HT
        uint8_t     **ludo_seed;
		int m;                         // size of a table
		int b;     	               // number of slots in a bucket
		int K;     	               // number of way
		int num_item;                  // number of inserted item
		int tmax;
		bool victim_flag;
		key_type victim_key;
		value_type victim_value;

        public:
                HTmap();
                HTmap(int way, int buckets, int hsize);
                HTmap(int way, int buckets, int hsize,int t);
                virtual ~HTmap();
                void clear();
                void expand();
                int updateseed(int numway_i, int bucket_iii);
                bool insert(key_type key,value_type value);
                bool direct_insert(key_type key,value_type value,int i, int ii);
                void stat();

		//LHS operator[]
                value_type& operator[](key_type key);
                //RHS operator[]
                const value_type operator[](key_type key) const  {return HTmap::query(key); }
                value_type query(key_type key);
                // return the value, the position (i,ii,p) and the number of accesses
                tuple<value_type,int,int,int,int> fullquery(key_type key);
                key_type  get_key(int i, int ii, int p);
                int count(key_type key);
                bool remove(key_type key);
                bool erase(key_type key) {return HTmap::remove(key);}
                long unsigned int size() {return (long unsigned int) num_item;}
                int get_nitem() {return num_item;}
                int get_size() {return K*b*m;}


       // iterators:
       //http://web.stanford.edu/class/cs107l/handouts/04-Custom-Iterators.pdf
       //http://www.cs.northwestern.edu/~riesbeck/programming/c++/stl-iterator-define.html

       //use private data of HTIterator class
       friend class HTIterator<key_type,value_type>;
       typedef HTIterator<key_type,value_type> iterator;

       iterator begin() {

           if(get_nitem()==0) return iterator(*this,0,0,0);
           int x=0;
           int y=0;
           int z=0;
           while(!present_table[x][y][z]){
               z++;
               if (z==m)  {y++; z=0;}
               if (y==b)  {x++; y=0; z=0;}
               if (x==K)  {x=0; y=0; z=0;}
           }
           return iterator(*this,x,y,z);
       }

       iterator end() {
           if(get_nitem()==0) return iterator(*this,0,0,0);
           int x=K-1;
           int y=b-1;
           int z=m-1;
           while(!present_table[x][y][z]){
               z--;
               if (z==-1) {y--; z=m-1;}
               if (y==-1) {x--; y=b-1; z=m-1;}
               if (x==-1) {x=K-1; y=b-1; z=m-1;}
           }
           return iterator(*this,x,y,z);
       }
};

template <typename key_type, typename value_type>
class HTIterator{
       private:
       HTmap<key_type,value_type> & myHT;
       int x,y,z;

       public:
       HTIterator(HTmap<key_type,value_type> & _ht, int _x, int _y, int _z) //constructor
           :myHT(_ht),x(_x),y(_y),z(_z) {}

       ~HTIterator(){} //distructor
       bool operator==(const HTIterator& other) const {
           if ((x==other.x) && (y==other.y) && (z==other.z))
               return true;
            return false;
       }
       bool operator!=(const HTIterator& other) const {
           if ((x==other.x) && (y==other.y) && (z==other.z))
               return false;
            return true;
       }

       pair<key_type,value_type> & operator*();
       HTIterator & operator++();
       HTIterator operator++(int);

};

template <typename key_type, typename value_type>
pair<key_type,value_type>& HTIterator<key_type,value_type>::operator*()
{
return myHT.table[x][y][z];
}

template <typename key_type, typename value_type>
HTIterator<key_type,value_type> & HTIterator<key_type,value_type>::operator++()
{
    if(myHT.get_nitem()==0) return *this;
    do
    {
        z++;
        if (z==myHT.m)  {y++; z=0;}
        if (y==myHT.b)  {x++; y=0; z=0;}
        if (x==myHT.K) {x=0; y=0; z=0;}
    }
    while(!myHT.present_table[x][y][z]);

    return *this;
}

template <typename key_type, typename value_type>
HTIterator<key_type,value_type> HTIterator<key_type,value_type>::operator++(int)
{
     HTIterator temp = *this ;
     this++;
     return temp;
}

/*
 * Constructor
 */

inline uint64 CityHash64WithSeed(int64_t key, uint64_t seed)
{
 return CityHash64WithSeed((const char *)&key,8,seed);
}


/*inline uint64 CityHash64WithSeed(std::pair<int64_t,int64_t> key, uint64_t seed) 
{
    char k[16];
    memcpy(k,key.first,8);
    memcpy(k+8,key.second,8);
    return CityHash64WithSeed(k,16,seed);
}*/

template <typename T>  
uint64 CityHash(T key, uint64_t seed) 
{
    char* k = reinterpret_cast<char*>(&key);
    return CityHash64WithSeed(k,sizeof(key),seed);
}

template <typename T>  
uint64 CityHash(std::string key, uint64_t seed) 
{
    return CityHash64WithSeed(key.c_str(),key.length(),seed);
}

template <typename T>  
uint8_t myCityHash(T key, uint8_t seed) 
{
    return (uint8_t)(CityHash<T>(key, (uint64_t) seed) & 0x3);
}

template <typename T> int myhash(T key, int i, int s) {
    uint64_t  val0;
    uint64_t  val1;
    uint64_t   val;
    int ss=s;

    val0=CityHash<T>(key,3015) % ss;
    val1=CityHash<T>(key,7793) % ss;
    if (val1==val0) {
        val1 = (val1 +1) % ss;
    }
    if (i==0) val=val0;
    if (i==1) val=val1;
    if (i>1)  val=CityHash<T>(key,2137+i) % ss;
    return (val %ss);


}


template <typename key_type, typename value_type>
HTmap<key_type,value_type>::HTmap(int way, int buckets, int hsize,int t)//t就是在插入的时候最多迁移几次.
{
  tmax=t;
  m = hsize; // size of memory
  b = buckets;
  K = way;
  num_item=0;   	// number of inserted item

  //allocation of HT memory Kxbxm
  present_table = new bool**[K];
  table = new pair<key_type,value_type>**[K];
  for (int i = 0;  i <K;  i++) {
      present_table[i] = new bool*[b];
      table[i]= new pair<key_type,value_type>*[b];
      for (int ii = 0;  ii <b;  ii++) {
          present_table[i][ii] = new bool[m];
          table[i][ii]= new pair<key_type,value_type>[m];
      }
  }

  ludo_seed = new uint8_t*[K];
  for (int i = 0; i < K; i++) {
      ludo_seed[i] = new uint8_t[m];
      for (int j = 0; j < m; j++) {
          ludo_seed[i][j] = 0;
      }
  }

  clear();
}

template <typename key_type, typename value_type>
HTmap<key_type,value_type>::HTmap(int way, int buckets, int hsize)
{
    HTmap<key_type,value_type>::HTmap(way,buckets,hsize,1000);
}


template <typename key_type, typename value_type>
HTmap<key_type,value_type>::HTmap()
{
    HTmap<key_type,value_type>::HTmap(2,4,1024*1024,1000);
}

/*
 * Distructor
 */
template <typename key_type, typename value_type>
HTmap<key_type,value_type>::~HTmap()
{
    for (int i = 0;  i <K;  i++){
        for (int ii = 0;  ii < b;  ii++){
            delete[] present_table[i][ii];
            delete[] table[i][ii];
        }
        delete[] present_table[i];
        delete[] table[i];
    }
    delete[] present_table;
    delete[] table;
}


/*
 * Clear
 */

template <typename key_type, typename value_type>
void HTmap<key_type,value_type>::clear()
{
	num_item=0;
        victim_flag=false;
        for (int i = 0;  i <K;  i++) {
            for (int ii = 0;  ii <b;  ii++)
                for (int iii = 0;  iii <m;  iii++){
                present_table[i][ii][iii]=false;
                }
    }
}


/*template <typename key_type, typename value_type>
HTmap<key_type,value_type>::expand()
{
  printf("expand table\n");
  //copy the content in a temp table
  bool*** temp_present_table = new bool**[K];
  pair<key_type,value_type>*** temp_table = new pair<key_type,value_type>**[K];
  for (int i = 0;  i <K;  i++) {
      temp_present_table[i] = new bool*[b];
      temp_table[i]= new pair<key_type,value_type>*[b];
      for (int ii = 0;  ii <b;  ii++){
          temp_present_table[i][ii] = new bool[m];
          temp_table[i][ii]= new pair<key_type,value_type>[m];
          for (int iii = 0;  iii <m;  iii++){
          temp_present_table[i][ii][iii] = present_table[i][ii][iii];
          temp_table[i][ii][iii]= table[i][ii][iii];
          }
      }
  }
  //delete the old table
  HTmap<key_type,value_type>::~HTmap();
  //create a new table
  HTmap<key_type,value_type>::HTmap(K,b,2*m);
  clear();
  //copy the content in the new table
  for (int i = 0;  i <K;  i++) {
      for (int ii = 0;  ii <b;  ii++){
          for (int iii = 0;  iii <m;  iii++){
          if (temp_present_table[i][ii][iii])
          HTmap<key_type,value_type>::insert(temp_table[i][ii][iii]= table[i][ii][iii]);
          }
      }
  }
}
*/



/*
 * Update Seed
 */
template <typename key_type, typename value_type>
int HTmap<key_type,value_type>::updateseed(int numway_i, int bucket_iii)
{
    bool occupied[4];
    int seed = ludo_seed[numway_i][bucket_iii];
    for (int s = 0; s < 255; s++) {     // loop 255 times instead of really from 0 -> 255.
      //seed ++;
      seed = (seed + 1) & 0xff;
      *(uint32_t *) occupied = 0U;  // memset zero to all of them.
      if (seed == ludo_seed[numway_i][bucket_iii])
        continue;
      bool success = true;
      
      for (int slot = 0; slot < b; ++slot) {
        if (present_table[numway_i][slot][bucket_iii]) {    // premice is you exists.
          uint8_t i = myCityHash<uint64_t>(table[numway_i][slot][bucket_iii].first, seed); // calculate the position of first.
          if (occupied[i]) {    // find we have key mapping to this place.
            success = false;
            break;
          } else {  
            occupied[i] = true; 
          }
        }
      }

      if (success) {
        unordered_map<uint64_t, int> pending_pairs;
        ludo_seed[numway_i][bucket_iii] = seed;
        for (int slot = 0; slot < b; ++slot) {
            if (present_table[numway_i][slot][bucket_iii]) {
                pending_pairs.insert(table[numway_i][slot][bucket_iii]);
                present_table[numway_i][slot][bucket_iii] = false;
            }   // insert these pairs in to this pending list.
        }
        for (auto it = pending_pairs.begin(); it != pending_pairs.end(); it++) {
            uint64_t key = it->first;
            uint8_t dslot = myCityHash<uint64_t>(key, seed);
            table[numway_i][dslot][bucket_iii] = {key,it->second}; //directly insert it.
            present_table[numway_i][dslot][bucket_iii] = true;
        }
        return seed;
      }

    }
    return -1;
}


/*
 * Insert
 */
template <typename key_type, typename value_type>
bool HTmap<key_type,value_type>::insert(key_type key,value_type value)
{
    //update value if exist
    if ((key==victim_key) && (victim_flag)) {
        victim_value=value;
        return true;
    }   // insert cannot support update.
    
    for (int i = 0;  i <K;  i++) {
        int p = myhash<key_type>(key,i,m);  // p is bucket index.
        int seed = ludo_seed[i][p]; 
        int ii = myCityHash<key_type>(key, seed);   
        if ((present_table[i][ii][p]) && (table[i][ii][p].first== key)) {
                table[i][ii][p].second=value;
                return true;
            }
        /*
        for (int ii = 0;  ii <b;  ii++)
            if ((present_table[i][ii][p]) && (table[i][ii][p].first== key)) {
                table[i][ii][p].second=value;
                return true;
            }
        */
    }

    // check if we need to grow the map
    //if(90*HTmap<key_type,value_type>::get_size()<100*HTmap<key_type,value_type>::get_nitem()) {
    //    HTmap<key_type,value_type>::expand();
    //}

    // try cuckoo. fine
    for (int t = 0;  t <= tmax;  t++) {

        // search for empty places
        for (int i = 0;  i <K;  i++){
            int p = myhash<key_type>(key,i,m);

        for (int ii = 0;  ii <b;  ii++)
            if (!present_table[i][ii][p]) {  //insert in an empty place
                present_table[i][ii][p] = true;
                table[i][ii][p]={key,value};
                updateseed(i, p);
                num_item++;
                return true;
            }
        /*
            for (int ii = 0;  ii <b;  ii++)
                if (!present_table[i][ii][p]) {  //insert in an empty place
                    present_table[i][ii][p] = true;
                    table[i][ii][p]={key,value};
                    num_item++;
                    return true;
                }
            */
        }
        //present[i][ii][p]: i-hash func table, ii-slots, p-bucket;
        // finally play the cuckoo;
        int j = rand() % K;
        int jj = rand() % b;
        int p = myhash<key_type>(key,j,m);
        key_type new_key = table[j][jj][p].first;
        value_type new_value = table[j][jj][p].second;
        table[j][jj][p]={key,value};
        updateseed(j, p);
        key=new_key;
        value=new_value;
    }
    printf("insertion failed\n");
    //cout << "key:<" << key.first <<","<< key.second <<">" <<endl;
    //cout << "value: " << value <<endl;

    victim_flag=true;
    victim_key=key;
    victim_value=value;
    return false;
}

/*
 * Direct Insert
 */
template <typename key_type, typename value_type>
bool HTmap<key_type,value_type>::direct_insert(key_type key,value_type value,int i, int ii)
{
    //return false if the key is in the victim cache
    if ((key==victim_key) && (victim_flag)) {
        printf("the key is in the victim cache\n");
        exit(1);
        return false;
    }

    int p = myhash<key_type>(key,i,m);

    //return false if the place is not free
    if (present_table[i][ii][p]) {
        printf("the place [%d][%d] is not free \n",i,ii);
        exit(1);
        return false;
    }

    present_table[i][ii][p] = true;
    table[i][ii][p]={key,value};
    num_item++;
    return true;
}


//LHS operator[]
template <typename key_type, typename value_type>
value_type& HTmap<key_type,value_type>::operator[](key_type key) {


    if (HTmap<key_type,value_type>::count(key)==0){ //insert if not exist
        HTmap<key_type,value_type>::insert(key,victim_value);
    }

    //update value
    if ((key==victim_key) && (victim_flag)) {
        //return result;
        return victim_value;
    }
    for (int i = 0;  i <K;  i++){
        int p = myhash<key_type>(key,i,m);
        int seed = ludo_seed[i][p];
        int ii = myCityHash<key_type>(key, seed);
        if ((present_table[i][ii][p]) && (table[i][ii][p].first== key)) {
                return table[i][ii][p].second;
            }
    }
    printf("ERROR in operator[]\n");
    exit(1);
    return victim_value;
}

/*
 * Query
 */
template <typename key_type, typename value_type>
value_type HTmap<key_type,value_type>::query(key_type key)
{
    if ((key==victim_key) && (victim_flag)) return victim_value;
    for (int i = 0;  i <K;  i++) {
        //for (int ii = 0;  ii <b;  ii++){
            int p = myhash<key_type>(key,i,m);
            //verprintf("query item in table[%d][%d] for p=%d and f=%d\n",p,jj,p,fingerprint);
            //verprintf("result is: %d\n",table[p][jj]);
            int seed = ludo_seed[i][p];
            int ii = myCityHash<key_type>(key, seed);
            if ((present_table[i][ii][p]) && (table[i][ii][p].first== key)) {
                return table[i][ii][p].second;
            }
            /*
            if ((present_table[i][ii][p]) &&  (table[i][ii][p].first== key)) {
                return table[i][ii][p].second;
            }*/
        //}
    }
    return victim_value;
}

/*
 * Full Query
 */
template <typename key_type, typename value_type>
tuple<value_type,int,int,int,int> HTmap<key_type,value_type>::fullquery(key_type key)
{
    int num_lookup=0;
    if ((key==victim_key) && (victim_flag)) return std::make_tuple(victim_value,-2,-2,-2,0);
    for (int i = 0;  i <K;  i++) {
        
            num_lookup++;
            int p = myhash<key_type>(key,i,m);
            //verprintf("query item in table[%d][%d] for p=%d and f=%d\n",p,jj,p,fingerprint);
            //printf("query item in table[%d][%d][%d]=%d\n",i,ii,p);
            //verprintf("result is: %d\n",table[p][jj]);
            int seed = ludo_seed[i][p];
            int ii = myCityHash<key_type>(key, seed);
            if ((present_table[i][ii][p]) && (table[i][ii][p].first== key)) {
                //table[i][ii][p].second=value;
                return make_tuple(table[i][ii][p].second,i,ii,p,num_lookup);
            }
            /*
            if ((present_table[i][ii][p]) &&  (table[i][ii][p].first== key)) {
                return make_tuple(table[i][ii][p].second,i,ii,p,num_lookup);
            }*/
        
    } 
    return std::make_tuple(victim_value,-1,-1,-1,num_lookup);
}


/*
 * Get key from index
 */
template <typename key_type, typename value_type>
key_type  HTmap<key_type,value_type>::get_key(int i, int ii, int p)
{
    if (present_table[i][ii][p])
        return table[i][ii][p].first;
    else
        return victim_key;
}

/*
 * Count
 */
template <typename key_type, typename value_type>
int HTmap<key_type,value_type>::count(key_type key)
{
    if ((key==victim_key) && (victim_flag)) {
        verprintf("match item in victim cache \n");
        return 1;
    }
    verprintf("query item in HT \n");
    for (int i = 0;  i <K;  i++) {
        //for (int ii = 0;  ii <b;  ii++){
            int p = myhash<key_type>(key,i,m);
            int seed = ludo_seed[i][p];
            int ii = myCityHash<key_type>(key, seed);
            verprintf("query item in table[%d][%d] for p=%d\n",i,ii,p);
            if ((present_table[i][ii][p]) &&  (table[i][ii][p].first== key)) {
                return 1;
            }
        //}
    }
    return 0;
}


template <typename key_type, typename value_type>
bool HTmap<key_type,value_type>::remove(key_type key) {
    if ((key==victim_key) && (victim_flag)){
            victim_flag=false;
            return true;
    }
    for (int i = 0;  i <K;  i++) {
        //for (int ii = 0;  ii <b;  ii++){
            int p = myhash<key_type>(key,i,m);
            int seed = ludo_seed[i][p];
            int ii = myCityHash<key_type>(key, seed);
            if ((present_table[i][ii][p]) &&  (table[i][ii][p].first== key)) {
                //printf("remove key %ld from [%d][%d]\n",key,i,ii);
                present_table[i][ii][p] = false;
                num_item--;
                return true;
            }
    }
return false;
}


/*
 * stat
 */

template <typename key_type, typename value_type>
void HTmap<key_type,value_type>::stat()
{
        int count=0;
        int pdf[5]={0,0,0,0,0};
        for (int i = 0;  i <K;  i++) {
	    for (int iii = 0;  iii <m;  iii++){
		count=0;
		for (int ii = 0;  ii <b;  ii++)
		    if (present_table[i][ii][iii]) count++; 
		pdf[count]++;
	    }
	}
}
