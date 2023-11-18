#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include <util/delay.h>
#include "ErrType.h"

#include "DIO_interface.h"

#include "CLCD_interface.h"
#include "CLCD_prv.h"
#include "CLCD_cfg.h"

static void VoidSendEnablePulse(void)
{
	DIO_u8SetPinValue(CLCD_u8CTRL_PORT,CLCD_u8E_PIN, DIO_u8PIN_HIGH);
	_delay_ms(2);
	DIO_u8SetPinValue(CLCD_u8CTRL_PORT,CLCD_u8E_PIN, DIO_u8PIN_LOW);
}


#if CLCD_u8BIT_MODE == FOUR_BIT
static void VoidSetHalfDataPort(uint8 Copy_u8Data)
{
	DIO_u8SetPinValue(CLCD_u8DATA_PORT,CLCD_u8D4_PIN, GET_BIT(Copy_u8Data,0));
	DIO_u8SetPinValue(CLCD_u8DATA_PORT,CLCD_u8D5_PIN, GET_BIT(Copy_u8Data,1));
	DIO_u8SetPinValue(CLCD_u8DATA_PORT,CLCD_u8D6_PIN, GET_BIT(Copy_u8Data,2));
	DIO_u8SetPinValue(CLCD_u8DATA_PORT,CLCD_u8D7_PIN, GET_BIT(Copy_u8Data,3));
}
#endif

void CLCD_VoidSendCmd(uint8 Copy_u8Cmd)
{
	/*Set RS pin to Low for command*/
	DIO_u8SetPinValue(CLCD_u8CTRL_PORT,CLCD_u8RS_PIN,DIO_u8PIN_LOW);

	/*Set RW pin to low for writing*/
#if CLCD_u8RW_conn_STS == DIO_CONNECTED
	DIO_u8SetPinValue(CLCD_u8CTRL_PORT,CLCD_u8RW_PIN,DIO_u8PIN_LOW);
#endif

#if CLCD_u8BIT_MODE == EIGHT_BIT
	/*Send the command*/
	DIO_u8SetPortValue(CLCD_u8DATA_PORT,Copy_u8Cmd);
	VoidSendEnablePulse();

#elif CLCD_u8BIT_MODE == FOUR_BIT
	VoidSetHalfDataPort(Copy_u8Cmd>>4);
	VoidSendEnablePulse();
	VoidSetHalfDataPort(Copy_u8Cmd);
	VoidSendEnablePulse();


#else
#error wrong CLCD_BIT_MODE configration option
#endif
}

void CLCD_VoidSendData(uint8 Copy_u8Data)
{
	/*Set RS pin to HIGH for data*/
	DIO_u8SetPinValue(CLCD_u8CTRL_PORT,CLCD_u8RS_PIN,DIO_u8PIN_HIGH);

	/*Set RW pin to low for writing*/
#if CLCD_u8RW_conn_STS == DIO_CONNECTED
	DIO_u8SetPinValue(CLCD_u8CTRL_PORT,CLCD_u8RW_PIN,DIO_u8PIN_LOW);
#endif

#if CLCD_u8BIT_MODE == EIGHT_BIT
	/*Send the Data*/
	DIO_u8SetPortValue(CLCD_u8DATA_PORT,Copy_u8Data);
	VoidSendEnablePulse();

#elif CLCD_u8BIT_MODE == FOUR_BIT
	/*Send the 4 Most significant bits of the data first*/
	VoidSetHalfDataPort(Copy_u8Data>>4);
	VoidSendEnablePulse();
	VoidSetHalfDataPort(Copy_u8Data);
	VoidSendEnablePulse();


#else
#error wrong CLCD_BIT_MODE configration option
#endif
}

void CLCD_VoidInti(void)
{
	/*wait for more than 30ms after power on*/
	_delay_ms(40);

	/*Function set command: 2 lines, Font size: 5x7*/
#if CLCD_u8BIT_MODE == EIGHT_BIT
	CLCD_VoidSendCmd(0b00111000);

#elif CLCD_u8BIT_MODE == FOUR_BIT
	VoidSetHalfDataPort(0b0010);
	VoidSendEnablePulse();
	VoidSetHalfDataPort(0b0010);
	VoidSendEnablePulse();
	VoidSetHalfDataPort(0b1000);
	VoidSendEnablePulse();

#endif

	/*display on off control : display on, cursor off, cursor blink off*/
	CLCD_VoidSendCmd(0b00001100);

	CLCD_VoidSendCmd(1);
}

uint8 CLCD_u8SendString(const char* Copy_pchString)
{
	uint8 Local_u8ErrorState=OK;

	if(Copy_pchString!=NULL)
	{
		/*Sending each character of the string using pointer math*/
		while(*Copy_pchString!='\0')
		{
			CLCD_VoidSendData(*Copy_pchString);
			Copy_pchString++;
		}
	}
	else
		Local_u8ErrorState=NULL_PTR_ERR;

	return Local_u8ErrorState;
}

void CLCD_VoidSendNumber(sint32 Copy_s32Number)
{
	uint8 u8Arr[10]={0};

	if(Copy_s32Number<0)
	{
		/*printing negative sign*/
		CLCD_VoidSendData(45);
		Copy_s32Number*=-1;
	}

	if(Copy_s32Number>0)
	{
		sint8 i;
		/*Saving digits of the number in array*/
		for(i=0;Copy_s32Number>0;i++)
		{
			u8Arr[i]=(uint8)(Copy_s32Number%10);
			Copy_s32Number/=10;
		}
		/*Sending each digit */
		for(sint8 k=(sint8)(i-1u);k>=0;k--)
		{
			CLCD_VoidSendData(48+u8Arr[(uint8)k]);
		}

	}

	/*Sending 0 if number =0*/
	else
		CLCD_VoidSendData(48);
}

void CLCD_VoidGoToXY(uint8 Copy_u8XPos,uint8 Copy_u8YPos)
{
	uint8 Local_u8Address;

	Local_u8Address= Copy_u8YPos * 0x40 + Copy_u8XPos ;

	/*Set bit 7 for setDDRAM address command*/
	SET_BIT(Local_u8Address,7);
	/*send address to CLCD*/
	CLCD_VoidSendCmd(Local_u8Address);
}

uint8 CLCD_u8SendSpecialCharacter(uint8 Copy_u8LocationNum, uint8* Copy_pu8Pattern,uint8 Copy_u8XPos,uint8 Copy_u8YPos)
{
	uint8 Local_u8ErrorState=OK;

	if(Copy_pu8Pattern!=NULL)
	{
		uint8 Local_u8Counter;
		uint8 Local_u8CGRAMAddress = Copy_u8LocationNum * 8;

		/*set bit 6 for CGRAM Address command*/
		SET_BIT(Local_u8CGRAMAddress,6);

		/*set CGRAM Address*/
		CLCD_VoidSendCmd(Local_u8CGRAMAddress);

		/*Write the input pattern inside CGRAM*/
		for(Local_u8Counter=0;Local_u8Counter<8u;Local_u8Counter++)
		{
			CLCD_VoidSendData(Copy_pu8Pattern[Local_u8Counter]);
		}

		/*Go Back to DDRAM*/
		CLCD_VoidGoToXY(Copy_u8XPos,Copy_u8YPos);

		/*Display the special pattern inside CGRAM*/
		CLCD_VoidSendData(Copy_u8LocationNum);

	}
	else
		Local_u8ErrorState=NULL_PTR_ERR;

	return Local_u8ErrorState;
}
