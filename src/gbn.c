#include "../include/simulator.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

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
// We create a buffer to store all the msgs
struct buffer
{
  struct msg message;
  struct buffer *next;
};

bool sender_state = true;
struct pkt *curr_packets;
int seq_num_A = 0;
int seq_num_B = 0;
int get_checksum(struct pkt *packet);
struct buffer *head = NULL;
struct buffer *tail = NULL;

int nextsequence = 0;       // sequence count for A
int available_packets = 0; // no of packets in A's window
int WINDOW = 0;

int window_init = 0; // this is the packet for which we are waiting for ack
int last = 0;         // last tranmitted packet from the window
int waiting_ack = 0;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message) struct msg message;
{
  // Creating new buffer using message
  printf("%s \n", message.data);
  struct buffer *new = (struct buffer *)malloc(sizeof(struct buffer));
  for (int i = 0; i < sizeof(new->message.data) / sizeof(new->message.data[0]); i++)
  {
    new->message.data[i] = '\0';
  }
  if (new == NULL)
  {
    printf("no enough memory\n");
  }
  else
  {
    new->next = NULL;
    strncpy(new->message.data, message.data, sizeof(message.data) / sizeof(message.data[0]));
    new->message.data[20] = '\0';
    //  Adding new buffer in the existing buffer
    if (tail == NULL)
    {
      printf("Tail is null\n");
      tail = new;
      head = new;
    }
    else
    {
      tail->next = new;
      tail = new;
    }
  }

  // Retreive the first message in the buffer
  struct buffer *curr_buffer = head;
  printf(curr_buffer->message.data);
  printf("\n");
  if (curr_buffer == NULL)
  {
    printf("No msg to process\n");
    return;
  }
  if (((last + 1) % WINDOW) == window_init)
  {
    return;
  }
  else
  {
    if (available_packets != 0) // increment last pointer by 1 if there is already packet on last
    {
      last = (last + 1) % WINDOW;
    }
  }
  strncpy(curr_packets[last].payload, curr_buffer->message.data, 20);
  curr_packets[last].acknum = 1;
  curr_packets[last].seqnum = nextsequence;
  curr_packets[last].checksum = get_checksum(&curr_packets[last]);
  nextsequence++;
  available_packets++;
  tolayer3(0, curr_packets[last]);
  if (window_init == last)
  {
    starttimer(0, 50.0);
  }

  // Setting head to next message
  head = head->next;
  if (head == NULL)
  {
    printf("Head is null\n");
    tail = NULL;
  }
  free(curr_buffer);
}

int get_checksum(struct pkt *pkt)
{
  int result = 0;
  if (pkt == NULL)
  {
    return result;
  }
  result = result + pkt->acknum;
  result = result + pkt->seqnum;
  int i = 0;
  while (i < 20)
  {
    result = result + (unsigned char)pkt->payload[i++];
  }
  return result;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet) struct pkt packet;
{
  if (packet.checksum != get_checksum(&packet))
  {
    printf("Wrong checksum at A\n");
    return;
  }
  if (packet.acknum != curr_packets[window_init].seqnum)
  {
    printf("Incorrect ACK at A\n");
    return;
  }
  printf("A_input\n");
  curr_packets[window_init].seqnum = -1; // set seq no of that packet to -1
  stoptimer(0);
  printf("Timer stopped \n");
  available_packets--; // decrement number of packets in window
  struct buffer *n = head;
  printf("popped \n");
  printf("%s msg \n", n->message.data);
  if (available_packets == 0)
  {
    while (n != NULL)
    {
      printf("pkt in window = 0");
      curr_packets[last];
      strncpy(curr_packets[last].payload, n->message.data, sizeof(n->message.data) / sizeof(n->message.data[0]));
      printf("msg copied");
      curr_packets[last].seqnum = nextsequence;
      curr_packets[last].acknum = 1;
      curr_packets[last].checksum = get_checksum(&curr_packets[last]);
      nextsequence++;
      // update the number of packets in window
      available_packets++;
      tolayer3(0, curr_packets[last]);
      starttimer(0, 50.0);
    }
  }
  else
  {
    printf("pkt in window != 0");
    window_init = (window_init + 1) % WINDOW;
    if (n != NULL)
    {
      last = (last + 1) % WINDOW;
      curr_packets[last];
      strncpy(curr_packets[last].payload, n->message.data, sizeof(n->message.data) / sizeof(n->message.data[0]));
      printf("msg copied");
      curr_packets[last].seqnum = nextsequence;
      curr_packets[last].acknum = 1;
      curr_packets[last].checksum = get_checksum(&curr_packets[last]);
      nextsequence++;
      // update the number of packets in window
      available_packets++;
      tolayer3(0, curr_packets[last]);
    }
  }
  if (head != NULL)
  {
    head = head->next;
    if (head == NULL)
    {
      printf("Head is NULL after pop");
      tail = NULL;
    }
  }
  free(n);
  if (window_init != last || available_packets == 1)
  {
    starttimer(0, 50.0);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("inside time\n");
  int i = window_init;
  for (;i != last;i = (i + 1) % WINDOW)
  {
    tolayer3(0, curr_packets[i]);
  }
  tolayer3(0, curr_packets[i]);

  /* If there is still some packets, start the timer again */
  if (window_init != last || available_packets == 1)
  {
    starttimer(0, 50.0);
  }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  WINDOW = getwinsize(); // initialize the window varibale
  curr_packets = malloc(sizeof(struct pkt) * WINDOW);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet) struct pkt packet;
{
  if (packet.checksum != get_checksum(&packet))
  {
    printf("Wrong Packet");
    return;
  }
  if (packet.seqnum == seq_num_B)
  {
    ++seq_num_B;
    tolayer5(1, packet.payload);
  }
  else
  {
    printf("pkt seq:%d\n", packet.seqnum);
    printf("out of order packet\n");
    if (packet.seqnum < seq_num_B)
    {
      printf("sent ack:%d\n", packet.seqnum);
      packet.acknum = packet.seqnum;
      packet.checksum = get_checksum(&packet);
      tolayer3(1, packet);
    }
  }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
}