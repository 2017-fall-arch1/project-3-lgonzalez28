
/** \File pongMain.c briefly this file make 
 * the main items of the program like
 * the ball and the bars the score and menssages
 * also describes every shape and call the methods
 */

#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <string.h>

#define GREEN_LED BIT6

AbRect rect = {abRectGetBounds, abRectCheck, {2,10}};

u_char player1Score = '0';
u_char player2Score = '0';

static int state = 0;

/* size and frame of the game **/
AbRectOutline fieldOutline = {
  abRectOutlineGetBounds, abRectOutlineCheck,
  {screenWidth/2-5, screenHeight/2-15}
};


Layer fieldLayer = {
  (AbShape *)&fieldOutline,
  {screenWidth/2, screenHeight/2},
  {0,0}, {0,0},
  COLOR_BLUE,
    0
};
/* ball color and position*/
Layer layer3 = {
  (AbShape *)&circle8,
  {(screenWidth/2), (screenHeight/2)}, 
  {0,0}, {0,0},   
  COLOR_BLUE,
  &fieldLayer,
};


/* player 1 bar color, position and size*/
Layer layer1 = {
  (AbShape *) &rect,
  {screenWidth/2-50, screenHeight/2+5},   
  {0,0}, {0,0},   
  COLOR_WHITE,
    &layer3
};


/* player 2 bar color, size and position**/
Layer layer2 = {
  (AbShape *)&rect,
  {screenWidth/2+50, screenHeight/2+5},
  {0,0}, {0,0}, 
  COLOR_WHITE,
  &layer1,
};


/** Moving layer

 * Velocity control
 */

typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

MovLayer ml1 = { &layer1, {0,3}, 0 }; 
MovLayer ml3 = { &layer3, {2,4}, 0 }; 
MovLayer ml2 = { &layer2, {0,3}, 0 };

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { // for each moving layer
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { // for each moving layer
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1],
		bounds.botRight.axes[0], bounds.botRight.axes[1]);

    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {

	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;

	for (probeLayer = layers; probeLayer;
	     probeLayer = probeLayer->next) { 
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break;
	  } 
	} 
	/** checks all the colums and rows**/
	lcd_writeColor(color);

      }
    } 
  }
}




void moveBall(MovLayer *ml, Region *fence1, MovLayer *ml2, MovLayer *ml3)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  int velocity;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++){

      if((shapeBoundary.topLeft.axes[axis] < fence1->topLeft.axes[axis]) ||

	 (shapeBoundary.botRight.axes[axis] > fence1->botRight.axes[axis])){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);

      }
      else if((abShapeCheck(ml2->layer->abShape, &ml2->layer->posNext, &ml->layer->posNext))){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }

      else if((abShapeCheck(ml3->layer->abShape, &ml3->layer->posNext, &ml->layer->posNext))){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }

      else if((shapeBoundary.topLeft.axes[0] < fence1->topLeft.axes[0])){
	newPos.axes[0] = screenWidth/2;
	newPos.axes[1] = screenHeight/2;
	player2Score = player2Score - 255;
      }
   
      else if((shapeBoundary.botRight.axes[0] > fence1->botRight.axes[0])){
	newPos.axes[0] = screenWidth/2;
	newPos.axes[1] = screenHeight/2;
	player1Score = player1Score - 255;
      }
      if(player1Score == '9' || player2Score == '9'){
	state = 1;
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void moveDown(MovLayer *ml, Region *fence)

{

  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
    for (axis = 1; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}
void moveUp(MovLayer *ml, Region *fence)
{

  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Sub(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 1; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
  /**bacgroung color Black**/
}u_int bgColor = COLOR_BLACK;
int redrawScreen = 1;   
Region fieldFence;


void main()
{
  P1DIR |= GREEN_LED;//Green led on when CPU on
  P1OUT |= GREEN_LED;

  configureClocks(); //clock start
  lcd_init();
  p2sw_init(15); //switches
  layerInit(&layer2);
  layerDraw(&layer2);
  layerGetBounds(&fieldLayer, &fieldFence);
  enableWDTInterrupts();   
  or_sr(0x8);
  
  u_int switches = p2sw_read();


  for(;;){
    while (!redrawScreen) { 
      P1OUT &= ~GREEN_LED; // Green led off 
      or_sr(0x10); // CPU OFF
    }
    P1OUT |= GREEN_LED; // Green led on when CPU on
    redrawScreen = 0;
    movLayerDraw(&ml3, &layer2);
    movLayerDraw(&ml2, &layer2);
    movLayerDraw(&ml1, &layer2);
    //board score and writting
    drawChar5x7(5, 5, player1Score, COLOR_BLUE, COLOR_BLACK); 
    drawChar5x7(115, 5, player2Score, COLOR_BLUE, COLOR_BLACK);
    drawString5x7(5, 150, "Pong<3", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(38, 5, "<3Points<3", COLOR_WHITE, COLOR_BLACK);
  }
}
/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;      
  count ++;
  u_int switches = p2sw_read();
  if(count == 10){
    switch(state){
    case 0:
      moveBall(&ml3, &fieldFence, &ml1, &ml2);
      break;
    case 1:
      layerDraw(&layer2);
      if(player1Score > player2Score)
	drawString5x7(4,80, "#1 WON #2 keep trying:P", COLOR_WHITE, COLOR_BLACK);
      else if(player1Score < player2Score)
	drawString5x7(4, 50, "#2 WON #1 try again:P", COLOR_WHITE, COLOR_BLACK);
      break;
    }
    
    if(switches & (1<<3)){
      moveUp(&ml2, &fieldFence);
    }
    if(switches & (1<<2)){
      moveDown(&ml2, &fieldFence);
    }
    if(switches & (1<<1)){
      moveUp(&ml1, &fieldFence);
    }
    if(switches & (1<<0)){
      moveDown(&ml1, &fieldFence);
    }
    redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;    
}
