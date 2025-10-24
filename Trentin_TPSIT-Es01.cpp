#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;            //sì, continuo a usarlo

const int ID_MAX = 10;
const int W_MAX = 5;
mutex mtx;                      //mutex è un oggetto di sincronizzazione tra thread che permette di proteggere le sezioni critiche del codice
condition_variable cv;          //condition variable è una variabile di sincronizzazione tra thread che permettere di farci varie operazioni, come farlo "dormire" finchè non viene notificato da un altro thread
atomic<bool> fine(false);       //atomic<bool> è una varibaile booleana thread safe.

class Task {
    private:
        int taskId;
        
    public:
        Task (int id): taskId(id) {}    /*creo il costruttore degli oggetti, che serve per creare nuovi oggetti ogni volta che la classe viene chiamata*/
                                        
        int getId() {      //metodo della classe Task che permette di prendere l'id della task
            return taskId;
        }
};

class Worker {
    private:
        int nWorker;
        thread trd_worker;          //coloro che farannno il lavoro
        queue<Task>& codaTask;          //attributi passati per riferimento, in quanto sto lavorando con thread e quindi non si possono fare copie
        mutex& mtx;             
        condition_variable& cv; 
        atomic<bool>& fine; 
    
    public:
        Worker(int nWorker, queue<Task>& codaTask, mutex& mtx, condition_variable& cv, atomic<bool>& fine)
        : nWorker(nWorker), codaTask(codaTask), mtx(mtx), cv(cv), fine(fine) {            //lista di inizializzazione che assegna agli attributi di Worker i suoi valori
         trd_worker = thread([this]() {     //passa i vari attributi per riferimento al thread di Worker. Per colpa della lambda function [this]() {} bisognerà usare this-> prima di ogni attributo. :D
            while (true) {
                Task task(0);       //assegno un valore a task, per far capire che è occupato.Ho messo 0 pk mi andava, ma potevo mettere anche 1084384 (forse)
                {
                    unique_lock<mutex> lock(this->mtx);       //blocca la coda per far lavorare in pace il worker. uso unique_lock in quanto più flessibile di lock_guard e permette di farci operazioni (usare i comandi condition_variable) :D
                    this->cv.wait(lock, [this]() { 
                        return !this->codaTask.empty() || this->fine;       //cv.wait blocca il thread finchè una condizione è vera, ovvero [this]. Dentro ai {} c'è la condizione da rispettare, ovvero quando ci sono task o se il programma è finito allora è true
                    });         //sì, questo si può scrivere tutto su una riga sola, ma così fa più figo
                    if (this->fine && this->codaTask.empty()) break;    //serve per controllare se la coda è vuota ed in quel caso fa break, va messo "fine" pk può essere che la coda sia vuota ma Master stia ancota generando tasks. il break ci fa uscire fal while(true
                    task = this->codaTask.front();        //assegna il task in fronte alla codaTask e la assegna a task, ovvero l'oggetto della classe Task
                    this->codaTask.pop();                 //serve per liberare la coda e permettere a nuovi thread di arrivare
                }
                this_thread::sleep_for(chrono::milliseconds(300));      //serve per controllare il thread corrente e farlo dormire per 0.3S. non serve a nulla, anzi è sbagliato ma serve per farci capire che il programma sta effettivamente facendo qualcosa
                cout << "Il worker n°" << this->nWorker << " ha finito la task n°" << task.getId() << endl;      //chiamo la funzione (metodo) di task per prendere il numero della task su cui ha lavorato
            }
        });
    }
        
        void join() {       //metodo della funzione Worker  
        if (trd_worker.joinable()) {        //se è joinable allora joini bro non è difficile ;D
            trd_worker.join();
        }
    }
};

class Master {
    private:
        vector<unique_ptr<Worker>> workers;     //creo il vettore workers, dove saranno contenuti i lavoratori. uso "unique_ptr" visto che ogni worker è un thread
        queue<Task>& codaTask;      
        mutex& mtx;
        condition_variable& cv;     //passo per riferimento queste variabili scritte fuori, esattamente come per la classe worker
        atomic<bool>& fine;
        
    public:
        Master(queue<Task>& coda, mutex& mtx, condition_variable& cv, atomic<bool>& fine)       //qua si passa per riferimento non solo pk sto lavorando con thread, ma anche pk sono attributi pesanti e copiandoli occuperei un sacco di memoria sullo stack (o IP non mi ricordo sono stanco è tardi)
        : codaTask(coda), mtx(mtx), cv(cv), fine(fine) {}       //la lista di inizializzazione e vine usata per impostano gli attriuti di Master
        
        void inizia() {     //metodo della funzione Master :D
            for (int i = 0; i < W_MAX; ++i){
                workers.push_back(unique_ptr<Worker>(new Worker(i, codaTask, mtx, cv, fine)));     //creo vettore workers e con push_back ci aggiungo tutte le varie informazioni. uso "unique_ptr" pk ogni worker è un thread e quindi non si possono fare copie
            }
            for (int i = 0; i < ID_MAX; ++i){
                lock_guard<mutex> lock(mtx);        //blocca il mutex
                codaTask.push(Task(i));
            }
            cv.notify_all();    //notifica a tutti i worker che ci sono nuove task su cui lavorare
            
            fine = true;    //controlla se la flag è cambiata, ed in quel caso notifica a tutti i lavoratori che è ora di pausa pranzo o di andare a casa 
            cv.notify_all();    //notifica ai worker che il programma è finito e bisogna quindi finire il task e poi chiudere il programma
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
        Si può anche usare [&]() {}, che è pure una lambda function, solo che cattura tutto senza dover mettere this->, però è meno sicura se le variabili locali escono dallo scopo
*/