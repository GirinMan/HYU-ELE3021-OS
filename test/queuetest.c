#include <stdio.h>
#define NPROC 5

struct queue{
  struct proc* arr[NPROC];
  int head;
  int tail;
  int num;
};

void initQueue(struct queue* q){
  q->head = q->tail = q->num = 0;
}

int dequeue(struct queue* q){
  struct proc* elem;
  if(q->num < 1) return -1;
  else if (q->num == 1){
    elem = q->arr[q->head];
    initQueue(q);
  }
  else{
    q->num--;
    elem = q->arr[q->head];
    if(q->head == NPROC-1){
      q->head = 0;
    }
    else{
      q->head++;
    }
  }

  return elem;
}

void enqueue(struct queue* q, struct proc* elem){
  if(q->num == 0){
    initQueue(q);
  }
  else if(q->num == NPROC){
    return;
  }
  else{
    if(q->tail == NPROC - 1){
      q->tail = 0;
    }
    else{
      q->tail++;
    }
  }
  q->arr[q->tail] = elem;
  q->num++;
}

void printQueue(struct queue q){
    printf("Queue: num=%d head=%d tail=%d arr=[ ", q.num, q.head, q.tail);
    for(int i = q.head; i != q.tail; i++){
        if(i == NPROC){
            if(q.tail == 0) break;
            i = 0;
        }
        printf("%d, ", q.arr[i]);
    }
    if(q.num > 0) printf("%d", q.arr[q.tail]);
    printf(" ]\n");
}

int main(void){

    struct queue q1;
    initQueue(&q1);

    printQueue(q1);

    while(1){
      char cmd;
      int input;
      scanf("%c", &cmd);

      switch (cmd)
      {
      case 'e':
        scanf("%d", &input);
        printf("enqueue %d\n", input);
        enqueue(&q1, input);
        printQueue(q1);
        break;
      case 'd':
        printf("dequeue %d\n", dequeue(&q1));
        printQueue(q1);
        break;
      default:
        break;
      }

      
    }
    
    return 0;
}