#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <signal.h>
using namespace std;



class automat {
private:
	string s; //input word
	struct state {
		string sym;
		state *p[128]; 
		bool fin;
	};
	vector<struct state> states;
public:
	automat(string w) { //Constructing automat
		s = w;
		size_t wlen = s.size();

		states[wlen].fin = true;
		for (int i = 0; i <= wlen; i++) { //Constructing states
			state cur_st;
			string pr_word;
			if (i != 0) {
				for (int j = 0; j < i; j++) {
					pr_word.push_back(s[j]);
				}
				cur_st.sym = pr_word;
			}
			states.push_back(cur_st);
		}
		for (int i = 0; i <= wlen; i++) { //Nulling arrows and finaling bool
			for (int j = 0; j < 128; j++) {
				states[i].p[j] = &states[0];
			}
			states[i].fin = false;
		}
		states[wlen].fin = true;

		for (int i = 1; i <= wlen; i++) { // Backward arrows 
			for (int j = 0; j < 128; j++) {
				string prefix = states[i].sym;
				prefix.push_back(j);
				string pr_rev;

				int n = prefix.size();
				for (int k = 0; k < n; k++) {
					pr_rev.push_back(prefix[n - 1 - k]);
				}
				for (int k = 0; k < i; k++) {
					pr_rev.pop_back();
					string pr_revrev;
					for (int l = 0; l < pr_rev.size(); l++) {
						pr_revrev += pr_rev[pr_rev.size() - 1 - l];
					}
					for (int l = 0; l < pr_revrev.size(); l++) {
						if (pr_revrev[l] != states[i - k].sym[l]) {
							break;
						}
					}
					states[i].p[j] = &states[i - k];
					break;
				}
			}
		}
		for (int i = 0; i < wlen; i++) { // Forward arrows
			char c;
			size_t length;
			length = states[i + 1].sym.size();
			c = states[i + 1].sym[length - 1];
			states[i].p[c] = &states[i + 1];
		}

	}
	void find_str (string line, string path, int row) { // Finding a word in a line
		state *cur_st = &states[0];
		int line_size = line.size();
		for (int i = 0; i < line_size; i++) {
			if (line[i] >= 0) { //escaping negative chars (many of them are in non-text files)
				cur_st = cur_st->p[line[i]];
			}
			else {
				cur_st = &states[0];
			}
			if (cur_st-> fin == true) {
				cout << " Substring is in file: " << path << " in row : " << row << endl;
				cout << "String:" << line << endl;
				//break;
			}
		}
	}
};



int findstr( automat & word_kmp, mutex & kmp_lock, vector<string> & fl_names, std::vector<string>::iterator & cur, mutex & _cur_lock,  mutex & _lock_fl_names, mutex & _lock_output){
	string s;
	int count;
	bool b = false;
	
	while (1){
		_cur_lock.lock();
		//_lock_fl_names.lock();
		if (cur != fl_names.end())
			b = true;
		//_lock_fl_names.unlock();
		_cur_lock.unlock();
		while (b) {
			//_lock_fl_names.lock();
			_cur_lock.lock();
			string filename = *cur;
			++cur;
			_cur_lock.unlock();
			ifstream file (filename);
			if (!file) {
				//_lock_fl_names.unlock();
				_lock_output.lock();
				//cout << "File is not open" << endl;
				_lock_output.unlock();
				_cur_lock.lock();
				++cur;
				_cur_lock.unlock();
			
				return (-1);
				}

			else {
				vector <string> file_copy;
				while (getline(file, s)){
					file_copy.push_back(s);
				}
				//_lock_fl_names.unlock();

				//_cur_lock.lock();
				//string filename = *cur;
				//++cur;
				//_cur_lock.unlock();

				//cout << ct << endl;
				//ct++;
				//if (filename == "/home/ann/Desktop/infa/text.txt") cout << "heeeelp" << endl;
				for (int j = 0; j < file_copy.size(); j++){
                    kmp_lock.lock();
                    word_kmp.find_str(file_copy[j], filename, j );
                    kmp_lock.unlock();
				}
			
				file.close();
			//return 0;
			}		
		}
	}
	return 0;
}



int findpath_th(const string & path, // where we started
  vector<string>& fl_names, //
  vector <string> &dir_names, 
  std::vector<string>::iterator & cur_dir, mutex & cur_dir_lock,//
  int n, mutex & dir_lock, mutex & fl_names_lock, mutex & count_lock, int & count){
	
	DIR *dir;	
    struct dirent *entry;
	bool b = true;
	
	while (1){
		cur_dir_lock.lock();
		count_lock.lock();
		if (cur_dir == dir_names.end() && count == 0){
			return 0;
		}
		cur_dir_lock.unlock();
		count_lock.unlock();
	    
		count_lock.lock();
		count ++;
		count_lock.unlock();

		cur_dir_lock.lock();
		string dir_str = *cur_dir;
		++cur_dir;
		cur_dir_lock.unlock();

	    if((dir=opendir((dir_str).c_str())) != NULL){
			//cout << "fine" << endl;

	    while((entry = readdir(dir)) != NULL){
			//cout << "find" << endl;
			    struct stat st;			
    			string filename = path + "/" + entry->d_name;
		    	//cout << filename << endl;
			   	if(strcmp(entry->d_name, ".") == 0)
			    	continue;
    			if(strcmp(entry->d_name, "..") == 0)
	    			continue;
		    	lstat(filename.c_str(), &st);
			    if(S_ISDIR(st.st_mode)){
				    //It is a directory

	                dir_lock.lock();
    	            dir_names.push_back(filename);
        	        dir_lock.unlock();
        	        if (n) break;
					//dir_lock.lock();
					cur_dir_lock.lock();
					string copy_dirname = *cur_dir;
					//++cur_dir;
					cur_dir_lock.unlock();
					//dir_names.erase(dir_names.begin());
					//dir_lock.unlock();
					int f = findpath_th(copy_dirname, fl_names, dir_names, cur_dir, cur_dir_lock, n, dir_lock, fl_names_lock, count_lock, count);
		    	}
		    	else if(S_ISREG(st.st_mode)){
					//It is a file
	                fl_names_lock.lock();
				    fl_names.push_back(filename);
    	            fl_names_lock.unlock();
		   		}
            	closedir(dir);
  			//	return 0;	    		
			} 
		}
		else {
			return -1;
		}
		count_lock.lock();
		count--;
		count_lock.unlock();
		
		
    }
	
	return 0;
}





int findpath(const string & path, vector<string>& fl_names, int n){
	DIR *dir;
	if((dir=opendir((path).c_str())) != NULL){
		//cout << "fine" << endl;
		struct dirent *entry;
		while((entry = readdir(dir)) != NULL){
			
			struct stat st;			
			string filename = path + "/" + entry->d_name;
			//cout << filename << endl;
			if(strcmp(entry->d_name, ".") == 0)
				continue;
			if(strcmp(entry->d_name, "..") == 0)
				continue;
			lstat(filename.c_str(), &st);
			if(S_ISDIR(st.st_mode)){
				//It is a directory
				//cout << n << endl;
                if (n) break;
					//else findfile(filename, find, n); for search in 1 path
				
				else 
				{
					//cout << "dir" << endl;
					findpath(filename, fl_names, n);
				}

			}
			else if(S_ISREG(st.st_mode)){
				//It is a file
				
				fl_names.push_back(filename);
			}
	  		//	return 0;
		}
	}
	closedir(dir);
	return 0;
}

int main(int argc, char** argv){
	if(argc > 5) {
		printf("Incorect number of arguments in %s\n", argv[0]);
		return 1;
	}

    mutex kmp_lock;
	mutex dir_lock;
	mutex count_lock;
	mutex _lock_fl_names;
    mutex _lock_output;
	mutex _cur_lock;
	mutex cur_dir_lock;
    vector <string> dir_names;
	vector <string> fl_names; 
	vector <thread> v_th;

	
	string str; //pattern
	string dir_;
	int n = 0; // width 1
	int t_pth = 1;
	int count = 0;

	string ttt;
	size_t posen;
	size_t poset ;
    size_t posep ;
	size_t posesl;
	
    for (int i = 1; i < argc; i++){
		string s = string(argv[i]);
		size_t o = s.front();
        posen = s.find("-n");
		poset = s.find("-t");
        posep = s.find("0");
        posesl = s.find("/");
        //std::cout << poset << ' ' << string::npos << endl;
        if ((posen == string::npos && poset == string::npos &&  posesl == string::npos)
			&& (posen != o || poset != o || posep != o ||  posesl != o) ){
            str = s.erase(0, 1);
			//cout << str << endl;
		}
		else if (posen != string::npos) {
            n = 1;
            }
		else if(poset != string::npos) {
			for (size_t i = poset+2; i < s.size(); i++ )
				ttt += s[i]; 
			
			if (ttt != "")
				t_pth = atoi(ttt.c_str());
        }
		else {
			dir_ = s;
		}
    }

	//for (int i = 0; i < fl_names.size(); i++){
	//	cout << fl_names[i] << endl;
	//}
	//cout << dir_ << endl;

    automat word_kmp(str);

	if ( t_pth == 1){
		int oe = findpath(dir_, fl_names, n);
        //cout << oe << endl;
        //for (int i = 0; i < fl_names.size(); i++)
        //	cout << fl_names[i] << endl;
		std::vector<string>::iterator cur = fl_names.begin();
        cout << fl_names.size() << endl;
		findstr( word_kmp, kmp_lock, fl_names, cur, _cur_lock, _lock_fl_names, _lock_output);
		
        
	}
	
	else {
        std::vector<string>::iterator cur_dir = dir_names.begin();
        findpath(dir_, fl_names, n);
		//findpath_th(dir_, fl_names, dir_names, cur_dir, cur_dir_lock, n, dir_lock, _lock_fl_names, count_lock, count);
        //for (int i = 0; i < fl_names.size(); i++)
	    //    cout << fl_names[i] << endl;
        cout << fl_names.size() << endl;

//        for (int i = 0; i < t_pth; i++){

//          v_th.push_back(thread(findpath_th, ref(dir_names[i]), ref(fl_names), n, ref(dir_names), ref(dirlock), ref(_lock_fl_names), n));

//        } 
		//int oe = findpath(dir_, fl_names, n);
		//v_th.push_back(thread(findpath, ref(dir_), ref(fl_names), n));


		std::vector<string>::iterator cur = fl_names.begin();
    	for (int i = 1; i <= t_pth; i++){
			v_th.push_back(thread(findstr, ref (word_kmp), ref(kmp_lock), ref(str),
             ref(fl_names), ref(cur), ref(_cur_lock), ref(_lock_fl_names), ref(_lock_output)));		
		}
		//wait for finish

		for (int i = 0; i < t_pth; i++)
			v_th[i].join();
		
	}
	
	//fl_names.push_back("home/ann/Desktop/infa/ex.txt");
	
}
