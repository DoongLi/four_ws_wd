#include "math_process.h"



float ABS(float value)
{
	if(value>0)
		value = value;
	else if (value<0)
		value = -value;
	else
		value = 0;
	return value;
}

/**
  * @Bref   б�����ݴ���
  * @Param  slope  б�����ݽṹ��
  * @Param  target ����Ŀ��ֵ
  * @Param  scale  �ֶ�
  */
float Map(float input_min ,float input_max,float output_min ,float output_max,float value)
{
	float out;
	if(value>input_max)
	{
		value = input_max;
	}
	if(value<input_min)
	{
		value = input_min;
	}
	
	out = ((ABS(value - input_min))/(ABS(input_max - input_min))) * (ABS(output_max - output_min)) + output_min;
	
	if (out<output_min)
		  out = output_min;
	if(out>output_max)
			out = output_max;
	return out ;
}

/**
  * @Bref   б�����ݴ���
  * @Param  slope  б�����ݽṹ��
  * @Param  target ����Ŀ��ֵ
  * @Param  scale  �ֶ�
  * @Param  inital ��ǰֵ
  */
void slope_process(slope_t *slope, float target, float scale, float inital)
{
    slope->target = target;
		slope->tmp_target = inital;
		
        if(slope->tmp_target > slope->target)
            slope->add  -= scale;
        
        else if(slope->tmp_target < slope->target)
            slope->add  += scale;
				
				slope->tmp_target +=slope->add;
				
				if(ABS(slope->tmp_target - slope->target) < (scale*2))
				{
					slope->tmp_target = slope->target;
					slope->add = 0;
				}
				
				
     
    
}


/**
  *@Bref     ��������ƫ�����
  *@Param    set �趨ֵ get����ֵ circle_paraһȦ��ֵ
  *@Note     ���������£�ֱ�Ӽ����PID�е�ƫ��ֵ ���������� error �ķ����Ǵ�getָ��set���Ż�
  */
float circle_error(float set ,float get ,float circle_para)
{
    float error;
    if(set > get)
    {
        if(set - get> circle_para/2)
            error = set - get - circle_para;
        else
            error = set - get;
    }
    else if(set < get)
    {
        if(set - get<-1*circle_para/2)
            error = set - get +circle_para;
        else
            error = set - get;
    }
    else	error = 0;

    return error;
}


/**
  * @Bref   ���ݰ���ƫ�������޷�
  * @Param  ���ݱ���ָ�룬���ֵ��ƫ��������ʼֵ��
  */
void abs_limit(float *a, float ABS_MAX,float offset)
{
    if(*a > ABS_MAX+offset)
        *a = ABS_MAX+offset;
    if(*a < -ABS_MAX+offset)
        *a = -ABS_MAX+offset;
}



