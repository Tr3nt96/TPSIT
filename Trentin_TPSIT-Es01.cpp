#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

const int ID_MAX = 10;
const int W_MAX = 5;
mutex mtx;
condition_variable cv;
atomic<bool> fine(false);       //atomic<bool> è una varibaile booleana thread safe.

class Task {
    private:
        int taskId;
        
    public:
        Task (int id): taskId(id) {}    //Creo il costruttore degli oggetti, che serve per creare nuovi oggetti ogni volta che la classe viene chiamat
                                        //id(id) serve per passare a id il vaore di id
                                        
        int getId() {      //metodo della classe Task
            return taskId;
        }
};

class Worker {
    private:
        int nWorker;
        thread trd_worker;
        queue<Task>& codaTask;  
        mutex& mtx;             
        condition_variable& cv; 
        atomic<bool>& fine; 
    
    public:     //qua passo tutto per riferimento per evitare copie sullo stack.  cv per segliare i worker quando arrivano nuove task e una flag che chiude
        Worker(int nWorker, queue<Task>& codaTask, mutex& mtx, condition_variable& cv, atomic<bool>& fine)
            //nel costruttore sopra vengono passati il suo id, il riferimento alla coda dei task su cui andrà a lavorare, un mutex mtx che protegge l'accesso alla zona critica
        : nWorker(nWorker), codaTask(codaTask), mtx(mtx), cv(cv), fine(fine) {            //lista di inizializzazione che assegna agli attributi di Worker i suoi valori
         trd_worker = thread([this]() {     //passa i vari attributi per riferimento al thread di Worker. Grazie a [this]() {} bisognerà usare this-> prima di ogni attributo. CHE BELLO :O
            while (true) {
                Task task(0);       //assegno un valore a task, per far capire che è occupato
                {
                    unique_lock<mutex> lock(this->mtx);       //blocca la coda per far lavorare in pace il worker. uso unique_lock in quanto più flessibile di lock_guard e permette di usare cv.qualcosa() :D
                    this->cv.wait(lock, [this]() { 
                        return !this->codaTask.empty() || this->fine;       //cv.wait blocca il thread finchè una condizione è vera, ovvero [this]. Dentro ai {} c'è la condizione da rispettare, ovvero quando ci sono task o se il programma è finito allora è true
                    });     
                    if (this->fine && this->codaTask.empty()) break;    //serve per controllare se la coda è vuota ed in quel caso fa break, va messo "fine" pk può essere che la coda sia vuota ma Master stia ancota generando tasks
                                                            //break fa uscire dal while(true). se ha una sola istruzione si possonno evitare le parentesi
                    task = this->codaTask.front();        //assegna il task in fronte alla codaTask e la assegna a task
                    this->codaTask.pop();                 //rimuove il primo task di codaTask per darlo al worker liberando spazio
                }
                this_thread::sleep_for(chrono::milliseconds(300));      //serve per controllare il thread corrente e farlo dormire per 0.3S. non serve a nulla, anzi è sbagliato ma serve per far capire a te che runni il programma che sta effettivamente succedendo qualcosa
                cout << "Il bambino sottopagato di SHEIN n°" << this->nWorker << " ha finito la task n°" << task.getId() << endl;      //chiamo la funzione (metodo) di Task per prendere il numero della Task su cui ha lavorato
            }
        });
    }
        
        void join() {       //metodo della funzione Worker  :D
        if (trd_worker.joinable()) {        //se è joinable allora joini bro non è difficile
            trd_worker.join();
        }
    }
};

class Master {
    private:
        vector<unique_ptr<Worker>> workers;     //creo il vettore workers, dove saranno contenuti i lavoratori. uso "unique_ptr" visto che ogni worker è un thread
        queue<Task>& codaTask;      
        mutex& mtx;
        condition_variable& cv;     //passo per riferimento queste variabili scritte fuori
        atomic<bool>& fine;
        
    public:
        Master(queue<Task>& coda, mutex& mtx, condition_variable& cv, atomic<bool>& fine)       //passa gli attributi per riferimento così da non creare doppioni
        : codaTask(coda), mtx(mtx), cv(cv), fine(fine) {}       //i : sono la lista di inizializzazione e vine usata per impostano gli attriuti di Master 
        
        void inizia() {     //metodo della funzione Master :D
            for (int i = 0; i < W_MAX; ++i){
                workers.push_back(unique_ptr<Worker>(new Worker(i, codaTask, mtx, cv, fine)));     //creo vettore workers e con push_back ci aggiungo tutte le varie informazioni. make_unique<Worker> li passa già per riferimento, quindi scrivo solamente il nome delle variabili
            }
            for (int i = 0; i < ID_MAX; ++i){
                lock_guard<mutex> lock(mtx);        //blocca il mutex pk deve riosarsi
                codaTask.push(Task(i));             //aggiuge a codaTask la task numero i
            }
            cv.notify_all();    //notifica ai worker che c'è una nuova task su cui lui deve lavorare
            
            fine = true;        //controlla se la flag è cambiata, ed in quel caso notifica a tutti i worker che è ora di andare a pranzare o a casa (dipende da che ora runni il programma)
            cv.notify_all();    //notifica ai worker che il programma è finito e bisogna quindi finire il task e poi andare a casa
            for (auto& w : workers) {   //struttura for-each molto divertente e simpatica. con auto si lascia al compilatore capire dic he tipo è la variabile e con & la si passa per riferimento pk fa figo. quindi w sarà il riferimento di workers :D
                w->join();      //serve per chiamare il metodo join() dentro alla classe Worker
            }
        }
};

int main(){
    queue<Task> codaTask;   //prende gli oggetti della classe Task e con quella ci crea una coda
    Master m (codaTask, mtx, cv, fine);    //crea gli oggetti m chiamando la classe Master e gli passa la coda delle task, con cui poi lui lavora
    m.inizia();             //fa iniziare il programma
    
    return 0;
}

/*APPUNTI FATTI SERI su ciò che ho imparato da questo final boss:

  - Come dichiarare un Costruttore con lista di inizializzazione (IL MIGLIORE) ???
    
    --> class Persona {
        private:
            int a;
            string b;
            
        public:
            Persona (int a, string b): eta(a), nome(b) {
                vuoto oppure altre informazioni, tipo inizializzazione degli attributi
            }
    };

  - [this]() {} --> lambda function che cattura il puntatore delle variabili,a cui prima va scritta "this->"
        Si può anche usare [&]() {}, che è pure una lambda function, solo che cattura tutto senza dover mettere this->, però è meno sicura se le variabili locali escono dallo scope

*/

