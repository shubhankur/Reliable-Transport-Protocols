#include "../include/simulator.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
//We create a buffer to store all the msgs
struct buffer{
  struct msg message;
  struct buffer *next;
};

bool sender_state = true;
struct pkt curr_packet;
int seq_num_A = 0;
int seq_num_B = 0;

int get_checksum(struct pkt *packet);
void add_to_buffer(struct msg *m);

struct buffer *head = 0;
struct buffer *tail = 0;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  add_to_buffer(&message);
  struct buffer *curr_buffer = head;
  head = head -> next;
  if(head==0){
    tail == 0;
  }
  if(!sender_state) return;
  sender_state = false;
  strncpy(curr_packet.payload, head->message.data, 20);
  printf(curr_packet.payload);
  curr_packet.acknum = 1;
  curr_packet.seqnum = seq_num_A;
  curr_packet.checksum = get_checksum(&curr_packet);
  tolayer3(0, curr_packet);
  starttimer(0, 10.0);
}

void add_to_buffer(struct msg *m){
  //Creating new buffer using message
  struct buffer *new = malloc(sizeof(struct buffer)); 
  strncpy(new->message.data, m->data, 20);
  printf(new->message.data);
  //Adding new buffer in the existing buffer
  if(tail==0){
    tail = new;
    head = new;
  }
  else{
    tail->next = new;
    tail = new;
  }
  new->next=0;
}
int get_checksum(struct pkt *pkt){
    int result = 0;
    if(pkt == 0){
        return result;
    }
    result = result + pkt->acknum;
    result = result + pkt->seqnum;
    int i = 0;
    while(i<20){
      result = result + (unsigned char)pkt->payload[i++];
    }
    return result;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  if(packet.checksum!=get_checksum(&packet)){
    return;
  }
  if(packet.acknum!=seq_num_A){
    return;
  }
  seq_num_A=1-seq_num_A;
  sender_state = true;
  //stoptimer(0);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  if(sender_state==true){
    tolayer3(0, curr_packet);
    starttimer(0, 10.0);
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  if(packet.checksum != get_checksum(&packet))
  {
    return;
  }

  if(packet.seqnum != seq_num_B)
  {
    return;
  }
  /* normal package, deliver data to layer5 */
  else
  {
    seq_num_B = 1- seq_num_B;
    tolayer5(1, packet.payload);
  }

  /* send back ack */
  packet.acknum = packet.seqnum;
  packet.checksum = get_checksum(&packet);

  tolayer3(1, packet);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}
