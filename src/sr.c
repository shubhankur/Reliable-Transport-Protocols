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
int seq_num_A = 0;
int seq_num_B = 0;
int get_checksum(struct pkt *packet);
struct buffer *pop_msg();
struct buffer *head = NULL;
struct buffer *tail = NULL;

struct window_sr
{
  struct pkt p_items; // packet item
  int ack;
  int timeover;
};
struct window_sr *A_packets;
struct window_sr *B_packets;

int available_packets = 0;
int available_packets_B = 0;

int WINDOW = 0;

int window_init = 0;
int window_init_B = 0;

int last = 0;
int last_B = 0;

int temp = 0;
float current_time = 0;
bool is_timer_off = true;

int A_seqnum = 0;
int B_seqnum = 0;

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

  // check if window is full
  if (available_packets == WINDOW)
  {
    printf("window is full \n");
    return;
  }
  // Retreive the first message in the buffer
  struct buffer *curr_buffer = pop_msg();
  printf(curr_buffer->message.data);
  printf("\n");
  if (curr_buffer == NULL)
  {
    printf("No msg to process\n");
    return;
  }
  if (((last + 1) % WINDOW) == window_init)
  {
    printf("//////last is higher");
    return;
  }
  else
  {
    if (available_packets != 0) // increment last pointer by 1 if there is already packet on last
    {
      last = (last + 1) % WINDOW;
    }
  }
  strncpy(A_packets[last].p_items.payload, curr_buffer->message.data, 20);
  A_packets[last].p_items.acknum = 1;
  A_packets[last].p_items.seqnum = A_seqnum;
  A_packets[last].p_items.checksum = get_checksum(&A_packets[last].p_items);
  A_seqnum++;
  A_packets[last].timeover = current_time + 30.0;
  A_packets[last].ack = 0;
  available_packets++;
  tolayer3(0, A_packets[last].p_items);
  if (is_timer_off)
  {
    is_timer_off = false;
    printf("Timer on\n");
    starttimer(0, 1.0);
  }

  // Setting head to next message
//   head = head->next;
//   if (head == NULL)
//   {
//     printf("Head is null\n");
//     tail = NULL;
//   }
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
  if (packet.acknum == A_packets[window_init].p_items.seqnum)
  {
    printf("Matching seq_num at A_input\n");
    A_packets[window_init].ack = 1;
    available_packets--;
    if (available_packets == 0)
    {
      window_init = (window_init + 1) % WINDOW;
      last = (last + 1) % WINDOW;
      printf("Window is empty now\n");
      struct buffer *n = pop_msg();
      if (n != NULL)
      {
        strncpy(A_packets[last].p_items.payload, n->message.data, 20);
        free(n);
        A_packets[last].p_items.seqnum = A_seqnum;
        A_packets[last].p_items.acknum = 1;
        A_packets[last].p_items.checksum = get_checksum(&A_packets[last].p_items);
        A_seqnum++;
        A_packets[last].ack = 0;
        A_packets[last].timeover = current_time + 30.0;
        available_packets++;
        tolayer3(0, A_packets[last].p_items);
      }
      else
      {
        is_timer_off = true;
        stoptimer(0);
      }
    }
    else
    {
      int i = window_init;
      for (;i != last;)
      {
        int temp = (i + 1) % WINDOW;
        if (A_packets[temp].ack != 1)
        {
          break;
        }
        available_packets--;
        i = (i + 1) % WINDOW;

        if (i == last)
        {
          last = i;
        }
      }
      window_init = (i + 1) % WINDOW;
      if (available_packets == 0)
      {
        last = window_init;
      }
      // send packet from buffer
      struct buffer *n = pop_msg();
      if (n != NULL)
      {
        strncpy(A_packets[last].p_items.payload, n->message.data, 20);
        free(n);
        A_packets[last].p_items.seqnum = A_seqnum;
        A_packets[last].p_items.acknum = 1;
        A_packets[last].p_items.checksum = get_checksum(&A_packets[last].p_items);
        A_seqnum++;
        A_packets[last].ack = 0; // set ack to not received
        A_packets[last].timeover = current_time + 30.0;
        available_packets++; // increase the number of packets in the window
        tolayer3(0, A_packets[last].p_items);
      }
    }
  }
  else if (packet.acknum > A_packets[window_init].p_items.seqnum)
  {
    int i = window_init;
    for (;i != last;i = (i + 1) % WINDOW)
    {
      temp = (i + 1) % WINDOW;
      if (packet.acknum == A_packets[temp].p_items.seqnum)
      {
        A_packets[temp].ack = 1;
        break;
      }
    }
  }
//   if (head != NULL)
//   {
//     head = head->next;
//     if (head == NULL)
//     {
//       printf("Head is NULL after pop");
//       tail = NULL;
//     }
//   }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  current_time = current_time + 1.0;
  if (available_packets != 0)
  {

    // printf("pkt in windw:%d\n",pkt_in_window);
    int i = window_init;
    // printf("Window start:%d\n",pkt_in_window);
    while (i != last)
    {
      if (A_packets[i].ack == 0 && A_packets[i].timeover < current_time)
      {
        A_packets[i].timeover = current_time + 30.0;
        tolayer3(0, A_packets[i].p_items);
      }
      i = (i + 1) % WINDOW;
    }
    if (A_packets[i].ack == 0 && A_packets[i].timeover < current_time)
    {
      A_packets[i].timeover = current_time + 30.0;
      tolayer3(0, A_packets[window_init].p_items);
    }
  }
  starttimer(0, 1.0);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  WINDOW = getwinsize();
  A_packets = malloc(sizeof(struct window_sr) * WINDOW);
  int i =0;
  while (i < WINDOW)
  {
    A_packets[i].ack == 0;
    i++;
  }
  is_timer_off = true;
  starttimer(0, 1.0);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet) struct pkt packet;
{
  if (packet.checksum != get_checksum(&packet))
  {
    return;
  }

  if (packet.seqnum == B_seqnum)
  {
    B_seqnum = B_seqnum + 1;
    tolayer5(1, packet.payload);
    packet.acknum = B_seqnum - 1; /* resend the latest ACK */
    packet.checksum = get_checksum(&packet);
    tolayer3(1, packet);

    B_packets[window_init_B].timeover = (B_seqnum) + WINDOW - 1;

    window_init_B = (window_init_B + 1) % WINDOW;

    for (;B_packets[window_init_B].p_items.seqnum == B_seqnum;)
    {
      tolayer5(1, B_packets[window_init_B].p_items.payload);
      B_seqnum = B_seqnum + 1;
      B_packets[window_init_B].timeover = (B_seqnum) + WINDOW - 1;
      window_init_B = (window_init_B + 1) % WINDOW;
    }
  }
  else
  {
    if (packet.seqnum > B_seqnum)
    {
      if (packet.seqnum <= B_seqnum + WINDOW)
      {
        int m = 0;
        while ( m < WINDOW)
        {
          if (B_packets[m].timeover == packet.seqnum)
          {
            B_packets[m].p_items = packet;
            packet.acknum = packet.seqnum;
            packet.checksum = get_checksum(&packet);
            tolayer3(1, packet);
            break;
          }
          m++;
        }
      }
    }
    else
    {
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
  WINDOW = getwinsize();
  B_packets = malloc(sizeof(struct window_sr) * WINDOW);
  int i = 0;
  while ( i < WINDOW)
  {
    B_packets[i].timeover = i; 
    i++;
  }
}
struct buffer *pop_msg()
{
    struct buffer *p;
    /* if the list is empty, return NULL*/
    if (head == NULL)
    {
        return NULL;
    }

    /* retrive the first node*/
    p = head;
    head = p->next;
    if (head == NULL)
    {
        tail = NULL;
    }
    return p;
}