#include "st7789.h"
#include "st7789font.h"

extern osSemaphoreId_t DMA_SemaphoreHandle;

uint8_t SPI_WriteByte(uint8_t *TxData, uint16_t size) {
    osStatus_t result;
    //获取信号，如果上一个DMA传输完成
    //信号就能获取到，没有传输完成任务就挂起
    //等到传输完成再恢复
    result = osSemaphoreAcquire(DMA_SemaphoreHandle, 0xFFFF);
    if (result == osOK) {
        //获取成功
        return HAL_SPI_Transmit_DMA(&hspi1, TxData, size);
    } else {
        //获取失败
        return 1;
    }
}

//DMA 传输完成后会调用 SPI传输完成回调函数
//在该函数中我们释放信号
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == hspi1.Instance)
        osSemaphoreRelease(DMA_SemaphoreHandle);
}

/******************************************************************************
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(uint8_t dat) {
    LCD_CS_Clr();

#if defined HW_SPI
    SPI_WriteByte(&dat, 1);
#elif defined SW_SPI
    uint8_t i;
        for(i=0;i<8;i++)
        {
            LCD_SCLK_Clr();
            if(dat&0x80)
            {
            LCD_MOSI_Set();
            }
            else
            {
            LCD_MOSI_Clr();
            }
            LCD_SCLK_Set();
            dat<<=1;
        }
#elif defined HW_SPI_DMA
    //TODO: spi+dma 在这感觉没必要，因为是发送一个字节。
    //期望整体优化，改成一个w*h的数组整体使用dma发送
#endif

    LCD_CS_Set();
}

void LCD_Writ_Bus_SIZE(uint8_t dat, uint16_t size) {
    LCD_CS_Clr();

#if defined HW_SPI
    HAL_SPI_Transmit(&ST7789_LCD_SPI, &dat, size, 0xffff);
#elif defined SW_SPI
    uint8_t i;
        for(i=0;i<8;i++)
        {
            LCD_SCLK_Clr();
            if(dat&0x80)
            {
            LCD_MOSI_Set();
            }
            else
            {
            LCD_MOSI_Clr();
            }
            LCD_SCLK_Set();
            dat<<=1;
        }
#elif defined HW_SPI_DMA
    //TODO: spi+dma 在这感觉没必要，因为是发送一个字节。
    //期望整体优化，改成一个w*h的数组整体使用dma发送
#endif

    LCD_CS_Set();
}

/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(uint8_t dat) {
    LCD_DC_Set();//写数据
    LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(uint16_t dat) {
    LCD_DC_Set();//写数据
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);
}

void LCD_WR_DATA_SIZE(uint16_t dat, uint16_t size) {
    LCD_DC_Set();//写数据
    LCD_Writ_Bus_SIZE(dat >> 8, size);
    LCD_Writ_Bus_SIZE(dat, size);
}


/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(uint8_t dat) {
    LCD_DC_Clr();//写命令
    LCD_Writ_Bus(dat);
    LCD_DC_Set();//写数据
}


/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if (USE_HORIZONTAL == 0) {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(x1 + 34);
        LCD_WR_DATA(x2 + 34);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(y1);
        LCD_WR_DATA(y2);
        LCD_WR_REG(0x2c);//储存器写
    } else if (USE_HORIZONTAL == 1) {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(x1 + 34);
        LCD_WR_DATA(x2 + 34);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(y1);
        LCD_WR_DATA(y2);
        LCD_WR_REG(0x2c);//储存器写
    } else if (USE_HORIZONTAL == 2) {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(x1);
        LCD_WR_DATA(x2);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(y1 + 34);
        LCD_WR_DATA(y2 + 34);
        LCD_WR_REG(0x2c);//储存器写
    } else {
        LCD_WR_REG(0x2a);//列地址设置
        LCD_WR_DATA(x1);
        LCD_WR_DATA(x2);
        LCD_WR_REG(0x2b);//行地址设置
        LCD_WR_DATA(y1 + 34);
        LCD_WR_DATA(y2 + 34);
        LCD_WR_REG(0x2c);//储存器写
    }
}

/**
 * @brief 默认适配中景园1.47英寸屏幕-172*320
 * 			实际使用时需要更改成你自己的
 *
 */
void LCD_Init(void) {
    //LCD_GPIO_Init();//初始化GPIO
    LCD_BLK_Clr();//关闭背光

    LCD_RES_Clr();//复位
    HAL_Delay(50);
    LCD_RES_Set();
    HAL_Delay(50);

    LCD_BLK_Set();//打开背光

    LCD_WR_REG(0x11);
    HAL_Delay(10);
    LCD_WR_REG(0x36);
    if (USE_HORIZONTAL == 0)LCD_WR_DATA8(0x00);
    else if (USE_HORIZONTAL == 1)LCD_WR_DATA8(0xC0);
    else if (USE_HORIZONTAL == 2)LCD_WR_DATA8(0x70);
    else LCD_WR_DATA8(0xA0);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x33);

    LCD_WR_REG(0xB7);
    LCD_WR_DATA8(0x35);

    LCD_WR_REG(0xBB);
    LCD_WR_DATA8(0x35);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x2C);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x01);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x13);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x20);

    LCD_WR_REG(0xC6);
    LCD_WR_DATA8(0x0F);

    LCD_WR_REG(0xD0);
    LCD_WR_DATA8(0xA4);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xD6);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0xF0);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x29);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x3E);
    LCD_WR_DATA8(0x38);
    LCD_WR_DATA8(0x12);
    LCD_WR_DATA8(0x12);
    LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x30);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0xF0);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x0A);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x3E);
    LCD_WR_DATA8(0x36);
    LCD_WR_DATA8(0x14);
    LCD_WR_DATA8(0x14);
    LCD_WR_DATA8(0x29);
    LCD_WR_DATA8(0x32);

    LCD_WR_REG(0x21);

    LCD_WR_REG(0x11);
    HAL_Delay(120);
    LCD_WR_REG(0x29);

}

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color) {
    uint16_t i, j;
    LCD_Address_Set(xsta, ysta, xend, yend);//设置显示范围
    for (i = ysta; i < yend; i++) {
        for (j = xsta; j < xend; j++) {
            LCD_WR_DATA(color);
        }
    }
}

//在指定区域内填充指定颜色块
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色
void LCD_Color_Fill(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t *color) {
    uint16_t i, j;
    uint16_t height, width;
    width = x_end - x_start + 1;
    height = y_end - y_start + 1;

    LCD_Address_Set(x_start, y_start, x_end, y_end);//设置显示范围

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            LCD_WR_DATA(color[i * width + j]);
        }
    }
}


/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
    LCD_Address_Set(x, y, x, y);//设置光标位置
    LCD_WR_DATA(color);
}

/******************************************************************************
      函数说明：画线
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   线的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; //计算坐标增量
    delta_y = y2 - y1;
    uRow = x1;//画线起点坐标
    uCol = y1;
    if (delta_x > 0)incx = 1; //设置单步方向
    else if (delta_x == 0)incx = 0;//垂直线
    else {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0;//水平线
    else {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)distance = delta_x; //选取基本增量坐标轴
    else distance = delta_y;
    for (t = 0; t < distance + 1; t++) {
        LCD_DrawPoint(uRow, uCol, color);//画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance) {
            yerr -= distance;
            uCol += incy;
        }
    }
}


/******************************************************************************
      函数说明：画矩形
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   矩形的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}


/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
                color   圆的颜色
      返回值：  无
******************************************************************************/
void Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {
    int a, b;
    a = 0;
    b = r;
    while (a <= b) {
        LCD_DrawPoint(x0 - b, y0 - a, color);             //3
        LCD_DrawPoint(x0 + b, y0 - a, color);             //0
        LCD_DrawPoint(x0 - a, y0 + b, color);             //1
        LCD_DrawPoint(x0 - a, y0 - b, color);             //2
        LCD_DrawPoint(x0 + b, y0 + a, color);             //4
        LCD_DrawPoint(x0 + a, y0 - b, color);             //5
        LCD_DrawPoint(x0 + a, y0 + b, color);             //6
        LCD_DrawPoint(x0 - b, y0 + a, color);             //7
        a++;
        if ((a * a + b * b) > (r * r))//判断要画的点是否过远
        {
            b--;
        }
    }
}

// /******************************************************************************
//       函数说明：显示汉字串
//       入口数据：x,y显示坐标
//                 *s 要显示的汉字串
//                 fc 字的颜色
//                 bc 字的背景色
//                 sizey 字号 可选 16 24 32
//                 mode:  0非叠加模式  1叠加模式
//       返回值：  无
// ******************************************************************************/
// void LCD_ShowChinese(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
// {
// 	while(*s!=0)
// 	{
// 		if(sizey==12) LCD_ShowChinese12x12(x,y,s,fc,bc,sizey,mode);
// 		else if(sizey==16) LCD_ShowChinese16x16(x,y,s,fc,bc,sizey,mode);
// 		else if(sizey==24) LCD_ShowChinese24x24(x,y,s,fc,bc,sizey,mode);
// 		else if(sizey==32) LCD_ShowChinese32x32(x,y,s,fc,bc,sizey,mode);
// 		else return;
// 		s+=2;
// 		x+=sizey;
// 	}
// }

// /******************************************************************************
//       函数说明：显示单个12x12汉字
//       入口数据：x,y显示坐标
//                 *s 要显示的汉字
//                 fc 字的颜色
//                 bc 字的背景色
//                 sizey 字号
//                 mode:  0非叠加模式  1叠加模式
//       返回值：  无
// ******************************************************************************/
// void LCD_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
// {
// 	uint8_t i,j,m=0;
// 	uint16_t k;
// 	uint16_t HZnum;//汉字数目
// 	uint16_t TypefaceNum;//一个字符所占字节大小
// 	uint16_t x0=x;
// 	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;

// 	HZnum=sizeof(tfont12)/sizeof(typFNT_GB12);	//统计汉字数目
// 	for(k=0;k<HZnum;k++)
// 	{
// 		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
// 		{
// 			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
// 			for(i=0;i<TypefaceNum;i++)
// 			{
// 				for(j=0;j<8;j++)
// 				{
// 					if(!mode)//非叠加方式
// 					{
// 						if(tfont12[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
// 						else LCD_WR_DATA(bc);
// 						m++;
// 						if(m%sizey==0)
// 						{
// 							m=0;
// 							break;
// 						}
// 					}
// 					else//叠加方式
// 					{
// 						if(tfont12[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
// 						x++;
// 						if((x-x0)==sizey)
// 						{
// 							x=x0;
// 							y++;
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
// 	}
// }

// /******************************************************************************
//       函数说明：显示单个16x16汉字
//       入口数据：x,y显示坐标
//                 *s 要显示的汉字
//                 fc 字的颜色
//                 bc 字的背景色
//                 sizey 字号
//                 mode:  0非叠加模式  1叠加模式
//       返回值：  无
// ******************************************************************************/
// void LCD_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
// {
// 	uint8_t i,j,m=0;
// 	uint16_t k;
// 	uint16_t HZnum;//汉字数目
// 	uint16_t TypefaceNum;//一个字符所占字节大小
// 	uint16_t x0=x;
//   TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
// 	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//统计汉字数目
// 	for(k=0;k<HZnum;k++)
// 	{
// 		if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
// 		{
// 			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
// 			for(i=0;i<TypefaceNum;i++)
// 			{
// 				for(j=0;j<8;j++)
// 				{
// 					if(!mode)//非叠加方式
// 					{
// 						if(tfont16[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
// 						else LCD_WR_DATA(bc);
// 						m++;
// 						if(m%sizey==0)
// 						{
// 							m=0;
// 							break;
// 						}
// 					}
// 					else//叠加方式
// 					{
// 						if(tfont16[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
// 						x++;
// 						if((x-x0)==sizey)
// 						{
// 							x=x0;
// 							y++;
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
// 	}
// }


// /******************************************************************************
//       函数说明：显示单个24x24汉字
//       入口数据：x,y显示坐标
//                 *s 要显示的汉字
//                 fc 字的颜色
//                 bc 字的背景色
//                 sizey 字号
//                 mode:  0非叠加模式  1叠加模式
//       返回值：  无
// ******************************************************************************/
// void LCD_ShowChinese24x24(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
// {
// 	uint8_t i,j,m=0;
// 	uint16_t k;
// 	uint16_t HZnum;//汉字数目
// 	uint16_t TypefaceNum;//一个字符所占字节大小
// 	uint16_t x0=x;
// 	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
// 	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//统计汉字数目
// 	for(k=0;k<HZnum;k++)
// 	{
// 		if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
// 		{
// 			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
// 			for(i=0;i<TypefaceNum;i++)
// 			{
// 				for(j=0;j<8;j++)
// 				{
// 					if(!mode)//非叠加方式
// 					{
// 						if(tfont24[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
// 						else LCD_WR_DATA(bc);
// 						m++;
// 						if(m%sizey==0)
// 						{
// 							m=0;
// 							break;
// 						}
// 					}
// 					else//叠加方式
// 					{
// 						if(tfont24[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
// 						x++;
// 						if((x-x0)==sizey)
// 						{
// 							x=x0;
// 							y++;
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
// 	}
// }

// /******************************************************************************
//       函数说明：显示单个32x32汉字
//       入口数据：x,y显示坐标
//                 *s 要显示的汉字
//                 fc 字的颜色
//                 bc 字的背景色
//                 sizey 字号
//                 mode:  0非叠加模式  1叠加模式
//       返回值：  无
// ******************************************************************************/
// void LCD_ShowChinese32x32(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode)
// {
// 	uint8_t i,j,m=0;
// 	uint16_t k;
// 	uint16_t HZnum;//汉字数目
// 	uint16_t TypefaceNum;//一个字符所占字节大小
// 	uint16_t x0=x;
// 	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
// 	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//统计汉字数目
// 	for(k=0;k<HZnum;k++)
// 	{
// 		if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
// 		{
// 			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
// 			for(i=0;i<TypefaceNum;i++)
// 			{
// 				for(j=0;j<8;j++)
// 				{
// 					if(!mode)//非叠加方式
// 					{
// 						if(tfont32[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
// 						else LCD_WR_DATA(bc);
// 						m++;
// 						if(m%sizey==0)
// 						{
// 							m=0;
// 							break;
// 						}
// 					}
// 					else//叠加方式
// 					{
// 						if(tfont32[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
// 						x++;
// 						if((x-x0)==sizey)
// 						{
// 							x=x0;
// 							y++;
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
// 	}
// }

//
///******************************************************************************
//      函数说明：显示单个字符
//      入口数据：x,y显示坐标
//                num 要显示的字符
//                fc 字的颜色
//                bc 字的背景色
//                sizey 字号
//                mode:  0非叠加模式  1叠加模式
//      返回值：  无
//******************************************************************************/
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum;//一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    num = num - ' ';    //得到偏移后的值
    LCD_Address_Set(x, y, x + sizex - 1, y + sizey - 1);  //设置光标位置
    for (i = 0; i < TypefaceNum; i++) {
        if (sizey == 12) temp = Ascii_1206[num][i];               //调用6x12字体
        else if (sizey == 16)temp = Ascii_1608[num][i];         //调用8x16字体
        else if (sizey == 24)temp = Ascii_2412[num][i];         //调用12x24字体
        else if (sizey == 32)temp = Ascii_3216[num][i];         //调用16x32字体
        else return;
        for (t = 0; t < 8; t++) {
            if (!mode)//非叠加模式
            {
                if (temp & (0x01 << t))LCD_WR_DATA(fc);
                else LCD_WR_DATA(bc);
                m++;
                if (m % sizex == 0) {
                    m = 0;
                    break;
                }
            } else//叠加模式
            {
                if (temp & (0x01 << t))LCD_DrawPoint(x, y, fc);//画一个点
                x++;
                if ((x - x0) == sizex) {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}


/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y显示坐标
                *p 要显示的字符串
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowString(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    while (*p != '\0') {
        LCD_ShowChar(x, y, *p, fc, bc, sizey, mode);
        x += sizey / 2;
        p++;
    }
}


/******************************************************************************
      函数说明：显示数字
      入口数据：m底数，n指数
      返回值：  无
******************************************************************************/
uint32_t mypow(uint8_t m, uint8_t n) {
    uint32_t result = 1;
    while (n--)result *= m;
    return result;
}


/******************************************************************************
      函数说明：显示整数变量
      入口数据：x,y显示坐标
                num 要显示整数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey) {
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t sizex = sizey / 2;
    for (t = 0; t < len; t++) {
        temp = (num / mypow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                LCD_ShowChar(x + t * sizex, y, ' ', fc, bc, sizey, 0);
                continue;
            } else enshow = 1;

        }
        LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
    }
}


/******************************************************************************
      函数说明：显示两位小数变量
      入口数据：x,y显示坐标
                num 要显示小数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowFloatNum1(uint16_t x, uint16_t y, float num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey) {
    uint8_t t, temp, sizex;
    uint16_t num1;
    sizex = sizey / 2;
    num1 = num * 100;
    for (t = 0; t < len; t++) {
        temp = (num1 / mypow(10, len - t - 1)) % 10;
        if (t == (len - 2)) {
            LCD_ShowChar(x + (len - 2) * sizex, y, '.', fc, bc, sizey, 0);
            t++;
            len += 1;
        }
        LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
    }
}


/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标
                length 图片长度
                width  图片宽度
                pic[]  图片数组
      返回值：  无
******************************************************************************/
void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[]) {
    uint16_t i, j;
    uint32_t k = 0;
    LCD_Address_Set(x, y, x + length - 1, y + width - 1);
    for (i = 0; i < length; i++) {
        for (j = 0; j < width; j++) {
            LCD_WR_DATA8(pic[k * 2]);
            LCD_WR_DATA8(pic[k * 2 + 1]);
            k++;
        }
    }
}

/**
 * @brief   以一种颜色清空LCD屏
 * @param   color —— 清屏颜色(16bit)
 * @return  none
 */
void LCD_Clear(uint16_t color) {
    uint16_t i;
    uint8_t data[2] = {0};  //color是16bit的，每个像素点需要两个字节的显存

    /* 将16bit的color值分开为两个单独的字节 */
    data[0] = color >> 8;
    data[1] = color;
    LCD_Address_Set(0, 0, LCD_W - 1, LCD_H - 1);
    LCD_CS_Clr();
    for (i = 0; i < ((LCD_W) * (LCD_H)); i++) {
        SPI_WriteByte(data, 2);
    }
    LCD_CS_Set();
}

void LCD_disp_flush(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color_p) {
    int32_t y;
    LCD_Address_Set(x1, y1, x2, y2);
    LCD_CS_Clr();
//
//    for (y = y1; y <= y2; y++) {
//        if (osSemaphoreAcquire(DMA_SemaphoreHandle, 0xFFFF) == osOK) {
//            HAL_SPI_Transmit_DMA(&hspi1,color_p, (uint16_t) (x2 - x1 + 1) * 2);
//        }
//        color_p += (x2 - x1 + 1);
//    }

    LCD_CS_Set();
}
