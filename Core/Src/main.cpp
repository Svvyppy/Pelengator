#include "main.h"

#include "Hw.h"
#include "Peleng.hpp"
#include "cordic.h"

Peleng peleng;

int main(void)
{
    // Initialize Peleng DSP processor
    peleng.Init();

    while (1)
    {
        // Process ADC data and calculate envelope
        peleng.Process();
    }
    /* USER CODE END 3 */
}

inline void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) { peleng.DmaTransferCompleteCallback(hadc); }

inline void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) { peleng.DmaHalfTransferCallback(hadc); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM15)
    {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
       line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
