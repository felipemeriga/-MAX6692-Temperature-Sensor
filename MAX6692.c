/* includes */
#include "MAX6692.h"

/* fun��es */

/*
 * Fun��o de escrita rand�mica N�O BLoqueante (SB - Single Byte)
 * Escreve o byte (dado) no endere�o (ender) no dispositivo cuja
 * palavra de controle de 7 bits est� contida em SlaveAddr
 * A vari�vel de estados de uma inst�ncia desta m�quina deve ter seu
 * endere�o passado atrav�s do ponteiro Event.
 * Esta fun��o � n�o bloqueante. Retorna o estado atual de sua m�quina de estados
 * retorna o estado stopped ao terminar de escrever na p�gina da mem�ria
 */

TypeMAX6692Event MAX6692_WR(uint8_t dado, uint8_t ender, uint8_t SlaveAddr,
		TypeMAX6692Event* Event) {

	switch (*Event) {
	case GenStart:
		// Send START condition
		I2C_GenerateSTART(I2C1, ENABLE);
		*Event = wait_EV5;
		break;

	case wait_EV5:
		// Test on EV5 and clear it
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			*Event = wait_EV6;
			// Send EEPROM address for write
			I2C_Send7bitAddress(I2C1, SlaveAddr, I2C_Direction_Transmitter);
		}
		break;

	case wait_EV6:
		// Test on EV6 and clear it
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			*Event = wait_EV8_SendAddr;
			// Escreve o endere�o de mem�ria a ser escrito
			I2C_SendData(I2C1, ender);
		}
		break;

	case wait_EV8_SendAddr:
		// Test on EV8 and clear it
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING)
				|| I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			*Event = wait_EV8_SendData;
			// Escreve o endere�o de mem�ria a ser escrito
			I2C_SendData(I2C1, dado);
		}
		break;

	case wait_EV8_SendData:
		// Test on EV8 and clear it
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING)
				|| I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			*Event = Stopped;
			// Send STOP condition
			I2C_GenerateSTOP(I2C1, ENABLE);
		}
		break;

	default:
		break;
	}

	return *Event;

}

/*
 * Fun��o que l� um �nico byte e o retorna atrav�s do ponteiro ret_data.
 * O byte � lido do endere�o ender do registrador de temperatura requerido do MAX6692.
 * SlaveAddr � a palavra de controle de 7 bits deslocada um bit � esquerda.
 * Event aponta para a vari�vel que controla o estado da m�quina.
 * Esta fun��o � n�o bloqueante. Retorna o estado atual de sua m�quina de estados
 * Ao final, retorna o estado stopped, indicando que a leitura foi realizada
 */

TypeMAX6692Event MAX6692_RD(uint8_t* ret_data, uint8_t ender, uint8_t SlaveAddr,
		TypeMAX6692Event* Event) {

	switch (*Event) {
	case GenStart:
		// Habilita ACK
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		// Send START condition
		I2C_GenerateSTART(I2C1, ENABLE);
		*Event = wait_EV5;
		break;

	case wait_EV5:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			*Event = wait_EV6;
			// Send EEPROM address for write
			I2C_Send7bitAddress(I2C1, SlaveAddr, I2C_Direction_Transmitter);
		}
		break;

	case wait_EV6:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			*Event = wait_EV8_SendAddr;
			// Escreve o endere�o de mem�ria a ser lido
			I2C_SendData(I2C1, ender);
		}
		break;

	case wait_EV8_SendAddr:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING)
				|| I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			*Event = wait_EV5_New_Start;
			/* Send START condition a second time */
			I2C_GenerateSTART(I2C1, ENABLE);
		}
		break;

	case wait_EV5_New_Start:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			*Event = wait_ADDR_SET;
			/* Send EEPROM address for read */
			I2C_Send7bitAddress(I2C1, SlaveAddr, I2C_Direction_Receiver);
		}
		break;

	case wait_ADDR_SET:
		// Verifica se ADDR=1 mas n�o o reseta
		if (I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0x0002) {
			*Event = wait_EV6_B;
			/* Disable Acknowledgement */
			I2C_AcknowledgeConfig(I2C1, DISABLE); // Bit CR1.ACK<-0 apenas ap�s ADDR=1, mas antes de reset�-lo
		}
		break;

	case wait_EV6_B:
		// Verifica, al�m de outros, se ADDR==1 e o reseta
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			*Event = wait_EV7;
			/* Send STOP Condition */
			I2C_GenerateSTOP(I2C1, ENABLE);
		}
		break;

	case wait_EV7:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
			*Event = Stopped;
			/*Get the data from memory*/
			*ret_data = I2C_ReceiveData(I2C1);
		}
		break;

	default:
		break;
	}

	return *Event;

}

/*
 * Fun��o que envia o byte do registrador de configura��o, por�m sem escrever um dado.
 * Configura o registrador no endere�o (ender) no dispositivo cuja
 * palavra de controle de 7 bits est� contida em SlaveAddr
 * A vari�vel de estados de uma inst�ncia desta m�quina deve ter seu
 * endere�o passado atrav�s do ponteiro Event.
 * Esta fun��o � n�o bloqueante. Retorna o estado atual de sua m�quina de estados
 * retorna o estado stopped ao terminar de escrever na p�gina da mem�ria
 */

TypeMAX6692Event MAX6692_SB(uint8_t ender, uint8_t SlaveAddr,
		TypeMAX6692Event* Event) {

	switch (*Event) {
		case GenStart:
			// Send START condition
			I2C_GenerateSTART(I2C1, ENABLE);
			*Event = wait_EV5;
			break;

		case wait_EV5:
			// Test on EV5 and clear it
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
				*Event = wait_EV6;
				// Send EEPROM address for write
				I2C_Send7bitAddress(I2C1, SlaveAddr, I2C_Direction_Transmitter);
			}
			break;

		case wait_EV6:
			// Test on EV6 and clear it
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
				*Event = wait_EV8_SendAddr;
				// Escreve o endere�o de mem�ria a ser escrito
				I2C_SendData(I2C1, ender);
			}
			break;

		case wait_EV8_SendAddr:
			// Test on EV8 and clear it
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING)
					|| I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
				*Event = Stopped;
				// Send STOP condition
				I2C_GenerateSTOP(I2C1, ENABLE);
			}
			break;

		default:
			break;
		}

		return *Event;

}

/*
 * Fun��o que l� um �nico byte e o retorna atrav�s do ponteiro ret_data.
 * O byte � lido do endere�o do registrador anteriormente configurado em uma fun��o
 * MAX6692_RD ou MAX6692_WR.
 * SlaveAddr � a palavra de controle de 7 bits deslocada um bit � esquerda.
 * Event aponta para a vari�vel que controla o estado da m�quina.
 * Esta fun��o � n�o bloqueante. Retorna o estado atual de sua m�quina de estados
 * Ao final, retorna o estado stopped, indicando que a leitura foi realizada
 */

TypeMAX6692Event MAX6692_RB(uint8_t* ret_data, uint8_t SlaveAddr,
		TypeMAX6692Event* Event) {

	switch (*Event) {
	case GenStart:
		// Habilita ACK
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		// Send START condition
		I2C_GenerateSTART(I2C1, ENABLE);
		*Event = wait_EV5;
		break;

	case wait_EV5:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
			*Event = wait_ADDR_SET;
			/* Send EEPROM address for read */
			I2C_Send7bitAddress(I2C1, SlaveAddr, I2C_Direction_Receiver);
		}
		break;

	case wait_ADDR_SET:
		// Verifica se ADDR=1 mas n�o o reseta
		if (I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0x0002) {
			*Event = wait_EV6_B;
			/* Disable Acknowledgement */
			I2C_AcknowledgeConfig(I2C1, DISABLE); // Bit CR1.ACK<-0 apenas ap�s ADDR=1, mas antes de reset�-lo
		}
		break;

	case wait_EV6_B:
		// Verifica, al�m de outros, se ADDR==1 e o reseta
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			*Event = wait_EV7;
			/* Send STOP Condition */
			I2C_GenerateSTOP(I2C1, ENABLE);
		}
		break;

	case wait_EV7:
		if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
			*Event = Stopped;
			/*Get the data from memory*/
			*ret_data = I2C_ReceiveData(I2C1);
		}
		break;

	default:
		break;
	}

	return *Event;

}
